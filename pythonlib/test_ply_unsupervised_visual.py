import argparse
import os
import sys
import unittest

import numpy as np
from plyfile import PlyData, PlyElement

# Ensure the built extension is on the path when run from pythonlib/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "bin"))

# Add MSYS2/UCRT64 runtime DLL directories on Windows
from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy

from opfppy.ply_adapter import SplatSubGraph
from opfppy.colormap import build_label_legend, export_label_legend, labels_to_rgb_array, load_colormap

# ---------------------------------------------------------------------------
# CLI arguments for visual colorization.
#
# Parsed once at module load so values are available for both direct execution
# and unittest invocation with additional arguments.
# ---------------------------------------------------------------------------
def _parse_visual_args(
    default_blend_ratio: float = 0.8,
    default_colormap_mode: str = "round-robin",
    default_colormap_source: str = "glasbey_hv",
    default_legend_format: str = "json",
    default_render_mode: str = "blend",
) -> tuple[float, str, str, str, str]:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument(
        "--blend-ratio",
        type=float,
        default=default_blend_ratio,
        help="Fraction of label color vs original color (0.0–1.0, default 0.8).",
    )
    parser.add_argument(
        "--colormap-mode",
        type=str,
        default=default_colormap_mode,
        help="Colormap mapping mode: round-robin (default) or stretch.",
    )
    parser.add_argument(
        "--colormap-source",
        type=str,
        default=default_colormap_source,
        help=(
            "Colorcet colormap name (e.g. glasbey_hv) or colormap filename "
            "(.hex/.txt/.csv/.json)."
        ),
    )
    parser.add_argument(
        "--legend-format",
        type=str,
        default=default_legend_format,
        help="Legend sidecar format: json (default), csv, txt, or none.",
    )
    parser.add_argument(
        "--render-mode",
        type=str,
        default=default_render_mode,
        help="Color rendering mode: blend (default) or label-only.",
    )
    # Parse and remove custom args so unittest itself won't reject them.
    args, remaining = parser.parse_known_args()
    sys.argv = [sys.argv[0], *remaining]
    blend_ratio = args.blend_ratio
    if not (0.0 <= blend_ratio <= 1.0):
        raise ValueError(f"--blend-ratio must be in [0.0, 1.0], got {blend_ratio}")
    legend_format = str(args.legend_format).strip().lower()
    if legend_format not in {"json", "csv", "txt", "none"}:
        raise ValueError(
            f"--legend-format must be one of json/csv/txt/none, got {args.legend_format}"
        )
    render_mode = str(args.render_mode).strip().lower()
    if render_mode not in {"blend", "label-only"}:
        raise ValueError(
            f"--render-mode must be one of blend/label-only, got {args.render_mode}"
        )
    return blend_ratio, args.colormap_mode, args.colormap_source, legend_format, render_mode


_BLEND_RATIO, _COLORMAP_MODE, _COLORMAP_SOURCE, _LEGEND_FORMAT, _RENDER_MODE = _parse_visual_args()
_COLORMAP = load_colormap(_COLORMAP_SOURCE)


_HERE = os.path.dirname(__file__)
_SAMPLE_PLY = os.path.normpath(
    os.path.join(_HERE, "..", "..", "tools", "bridge-server", "sample.ply")
)
_OUTPUT_DIR = os.path.join(_HERE, "build", "ply_visual_check")


def _copy_nodes(src: opfpy.Subgraph, count: int | None = None) -> opfpy.Subgraph:
    n = src.nnodes if count is None else min(count, src.nnodes)
    out = opfpy.Subgraph(n)
    out.nfeats = src.nfeats
    out.nlabels = max(1, src.nlabels)

    for i in range(n):
        s = src.get_node(i)
        d = out.get_node(i)
        d.feat = s.feat
        d.truelabel = 0
        d.label = 0
        d.position = i
        d.pathval = 0.0
        d.pred = -1
        d.status = 0
        d.relevant = 0
        d.root = i
    return out


def _base_rgb(vertices: np.ndarray) -> np.ndarray:
    names = set(vertices.dtype.names or [])
    if {"red", "green", "blue"}.issubset(names):
        return np.column_stack([vertices["red"], vertices["green"], vertices["blue"]]).astype(np.float32)

    if {"f_dc_0", "f_dc_1", "f_dc_2"}.issubset(names):
        dc = np.column_stack([vertices["f_dc_0"], vertices["f_dc_1"], vertices["f_dc_2"]]).astype(np.float32)
        # Approximate RGB from SH DC term using common C0 factor.
        rgb = (0.5 + 0.282095 * dc) * 255.0
        return np.clip(rgb, 0.0, 255.0)

    if "opacity" in names:
        g = np.clip(vertices["opacity"].astype(np.float32) * 255.0, 0.0, 255.0)
        return np.column_stack([g, g, g])

    return np.full((len(vertices), 3), 127.0, dtype=np.float32)



# Write a PLY file using write_subgraph_to_ply_file, with label and regular properties, no color blending.
def _write_label_ply(
    src_ply: str,
    labels: list[int],
    dst_ply: str,
    feature_profile: str = "full",
) -> None:
    from opfppy.ply_adapter import write_subgraph_to_ply_file, subgraph_from_ply_file
    sg, metadata = subgraph_from_ply_file(src_ply, feature_profile=feature_profile)
    for i in range(sg.nnodes):
        sg.get_node(i).label = labels[i] if i < len(labels) else 0
    write_subgraph_to_ply_file(sg, dst_ply, metadata=metadata, feature_profile=feature_profile + "+label")


def _write_label_legend(labels: list[int], dst_ply: str) -> str | None:
    if _LEGEND_FORMAT == "none":
        return None
    legend_path = f"{os.path.splitext(dst_ply)[0]}.legend.{_LEGEND_FORMAT}"
    export_label_legend(
        path=legend_path,
        labels=labels,
        mode=_COLORMAP_MODE,
        cmap=_COLORMAP,
        include_unlabeled=False,
        colormap_source=_COLORMAP_SOURCE,
    )
    return legend_path


def _sanity_check_random_splats(
    source: SplatSubGraph,
    train: opfpy.Subgraph,
    sample_size: int = 300,
    seed: int = 1337,
) -> None:
    """Sanity-check random splats against raw PLY and OPF training fields.

    Verifies, on a random sample:
    - feature vectors are finite and aligned with raw PLY properties (x, opacity,
      scales, rotations, etc. when present in the active feature profile)
    - native OPF training fields (`radius`, `dens`, `pathval`) are finite
    - at least some training radii are strictly positive
    """
    ply = PlyData.read(source.metadata["source"])
    vertices = ply["vertex"].data

    n = source.nnodes
    take = min(sample_size, n)
    rng = np.random.default_rng(seed)
    sampled_idx = rng.choice(n, size=take, replace=False)

    feat_names = list(source.metadata.get("feature_names", []))
    feat_map = {name: idx for idx, name in enumerate(feat_names)}

    # Verify source features are finite and consistent with raw PLY data.
    for i in sampled_idx:
        node = source.get_node(int(i))
        feat = np.asarray(node.feat, dtype=np.float64)
        if feat.size != source.nfeats:
            raise AssertionError(f"Feature length mismatch at node {i}: {feat.size} != {source.nfeats}")
        if not np.isfinite(feat).all():
            raise AssertionError(f"Non-finite feature values at node {i}")

        row = vertices[int(i)]
        # Cross-check key properties directly against the decoded OPF feature vector.
        for prop in (
            "x", "y", "z",
            "opacity",
            "scale_0", "scale_1", "scale_2",
            "rot_0", "rot_1", "rot_2", "rot_3",
            "f_dc_0", "f_dc_1", "f_dc_2",
        ):
            if prop in feat_map and prop in row.dtype.names:
                got = float(feat[feat_map[prop]])
                exp = float(row[prop])
                if not np.isclose(got, exp, rtol=1e-6, atol=1e-7):
                    raise AssertionError(
                        f"Property mismatch for '{prop}' at node {i}: got={got}, exp={exp}"
                    )

    # Train graph sanity (native C++ bestk_min_cut/createArcs/computePDF path).
    train_take = min(sample_size, train.nnodes)
    train_idx = rng.choice(train.nnodes, size=train_take, replace=False)
    radii = np.asarray([float(train.get_node(int(i)).radius) for i in train_idx], dtype=np.float64)
    dens = np.asarray([float(train.get_node(int(i)).dens) for i in train_idx], dtype=np.float64)
    pathval = np.asarray([float(train.get_node(int(i)).pathval) for i in train_idx], dtype=np.float64)

    if not np.isfinite(radii).all():
        raise AssertionError("Non-finite radius values in training sample")
    if not np.isfinite(dens).all():
        raise AssertionError("Non-finite density values in training sample")
    if not np.isfinite(pathval).all():
        raise AssertionError("Non-finite pathval values in training sample")
    if np.count_nonzero(radii > 0.0) == 0:
        raise AssertionError("All sampled training radii are zero")

    # Minimal diagnostic summary for visual sanity debugging.
    print(
        "[sanity] train sample stats: "
        f"radius[min={radii.min():.6f}, max={radii.max():.6f}], "
        f"dens[min={dens.min():.6f}, max={dens.max():.6f}], "
        f"pathval[min={pathval.min():.6f}, max={pathval.max():.6f}]"
    )

    for prop in ("opacity", "scale_0", "scale_1", "scale_2", "rot_0", "rot_1", "rot_2", "rot_3"):
        if prop in feat_map:
            vals = np.asarray(
                [float(source.get_node(int(i)).feat[feat_map[prop]]) for i in sampled_idx],
                dtype=np.float64,
            )
            if not np.isfinite(vals).all():
                raise AssertionError(f"Non-finite values for property '{prop}' in sampled splats")
            print(
                f"[sanity] {prop}: min={vals.min():.6f}, "
                f"max={vals.max():.6f}, mean={vals.mean():.6f}"
            )


class TestPlyUnsupervisedVisual(unittest.TestCase):
    def test_cluster_and_colorize_full_and_compact(self, train_size: int | None = 300, sample_size: int = 30) -> None:
        if not os.path.isfile(_SAMPLE_PLY):
            self.skipTest(f"Sample PLY not found: {_SAMPLE_PLY}")

        os.makedirs(_OUTPUT_DIR, exist_ok=True)

        for profile in ("full", "compact"):
            source = SplatSubGraph.from_ply_file(_SAMPLE_PLY, feature_profile=profile)

            # Train clustering model on a subset to keep test runtime bounded.
            train = _copy_nodes(source, count=train_size)
            clf = opfpy.OPF()
            # Native LibOPF pipeline (C -> C++ port -> pybind):
            #   opf_BestkMinCut -> createArcs + PDF, then opf_OPFClustering.
            clf.bestk_min_cut(train, 1, 10)
            clf.cluster(train)

            # Random sample sanity checks for splat properties + native OPF fields.
            _sanity_check_random_splats(source, train, sample_size=sample_size, seed=42)

            for i in range(train.nnodes):
                node = train.get_node(i)
                if node.root == i:
                    node.truelabel = node.label + 1
            opfpy.propagate_cluster_labels(train)

            # Classify all points from the same source representation.
            test_all = _copy_nodes(source)
            clf.knn_classify(train, test_all)

            labels = [test_all.get_node(i).label for i in range(test_all.nnodes)]
            nlabels = len(set(labels))
            print(f"[{profile}] resulting labels: {nlabels}")

            self.assertEqual(len(labels), source.nnodes)
            self.assertGreater(nlabels, 0)

            out_path = os.path.join(_OUTPUT_DIR, f"sample_labels_{profile}.ply")
            _write_colorized_ply(_SAMPLE_PLY, labels, out_path)
            legend_path = _write_label_legend(labels, out_path)
            self.assertTrue(os.path.isfile(out_path))
            if legend_path is not None:
                self.assertTrue(os.path.isfile(legend_path))
                preview_entries = build_label_legend(
                    labels=labels,
                    mode=_COLORMAP_MODE,
                    cmap=_COLORMAP,
                    include_unlabeled=0 in labels,
                )[:5]
                preview_str = ", ".join(
                    f"{entry['label']}->{tuple(int(v) for v in entry['rgb'])}"
                    for entry in preview_entries
                )
                print(f"[{profile}] legend: {legend_path}")
                print(f"[{profile}] legend preview: {preview_str}")
            print(
                f"[{profile}] visual output: {out_path} "
                f"(blend_ratio={_BLEND_RATIO:.2f}, "
                f"render_mode={_RENDER_MODE}, "
                f"colormap_mode={_COLORMAP_MODE}, "
                f"colormap_source={_COLORMAP_SOURCE}, "
                f"legend_format={_LEGEND_FORMAT})"
            )


    def test_write_label_ply_both_profiles(self) -> None:
        """Write PLY with '+label' suffix for both profiles and sanity-check the output."""
        import io
        if not os.path.isfile(_SAMPLE_PLY):
            self.skipTest(f"Sample PLY not found: {_SAMPLE_PLY}")

        os.makedirs(_OUTPUT_DIR, exist_ok=True)

        for profile in ("full", "compact"):
            source = SplatSubGraph.from_ply_file(_SAMPLE_PLY, feature_profile=profile)
            n = source.nnodes

            # Assign cycling labels 1..5
            labels = [(i % 5) + 1 for i in range(n)]

            out_path = os.path.join(_OUTPUT_DIR, f"sample_label_only_{profile}.ply")
            _write_label_ply(_SAMPLE_PLY, labels, out_path, feature_profile=profile)

            # File must exist and be kept
            self.assertTrue(os.path.isfile(out_path), f"Output PLY not created: {out_path}")

            # Check PLY header contains 'label'
            with open(out_path, "rb") as f:
                header = b""
                while True:
                    line = f.readline()
                    header += line
                    if line.strip() == b"end_header":
                        break
            self.assertIn(b"label", header, f"[{profile}] 'label' not found in PLY header")

            # Check binary data: label field present and values match
            with open(out_path, "rb") as f:
                ply = PlyData.read(io.BytesIO(f.read()))
            arr = ply["vertex"].data
            self.assertIn("label", arr.dtype.names, f"[{profile}] 'label' column missing in vertex data")
            self.assertEqual(len(arr), n, f"[{profile}] vertex count mismatch")
            for i in range(n):
                self.assertEqual(
                    int(arr["label"][i]), labels[i],
                    f"[{profile}] label mismatch at node {i}: got {arr['label'][i]}, expected {labels[i]}"
                )

            # Check that feature columns are present
            expected_features = source.metadata.get("feature_names", [])
            for feat in expected_features[:5]:  # spot-check first 5 features
                self.assertIn(feat, arr.dtype.names, f"[{profile}] feature '{feat}' missing from PLY")

            print(f"[{profile}+label] output kept: {out_path} ({n} nodes, {len(arr.dtype.names)} columns)")


if __name__ == "__main__":
    unittest.main()
