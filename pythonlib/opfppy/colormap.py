"""Categorical colormap utilities for label visualization.

Features:
- Default Colorcet palette (glasbey_hv) with high categorical contrast.
- Two label mapping modes:
    - round-robin: repeats palette indices cyclically.
    - stretch: interpolates colors across the full palette extent.
- Generic colormap loading from Colorcet names or local files:
    - .hex/.txt: one hex color per line
    - .csv: rows with r,g,b (0-1 float or 0-255 int)
    - .json: list of hex strings, list of rgb triplets, or
                     dict with "colors"/"colormap" list.
"""

from __future__ import annotations

import csv
import json
import os
import colorcet as cc
import numpy as np

# Reserve index 0 as white (label ≤ 0 / unlabeled).
_UNLABELED: tuple[int, int, int] = (255, 255, 255)
_MODES = {"round-robin", "rounding-robin", "stretch"}


def _hex_to_rgb(hex_color: str) -> tuple[int, int, int]:
    s = hex_color.strip()
    if not s:
        raise ValueError("Empty hex color entry")
    if s.startswith("#"):
        s = s[1:]
    if len(s) != 6:
        raise ValueError(f"Hex color must have 6 chars, got '{hex_color}'")
    return (int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16))


def _rgb_to_uint8(rgb: list[float] | tuple[float, float, float]) -> tuple[int, int, int]:
    if len(rgb) != 3:
        raise ValueError(f"RGB entry must have 3 channels, got {rgb}")
    vals = [float(v) for v in rgb]
    # Support both 0..1 and 0..255 inputs.
    if max(vals) <= 1.0:
        vals = [v * 255.0 for v in vals]
    return tuple(int(round(np.clip(v, 0.0, 255.0))) for v in vals)


def _load_from_colorcet(name: str) -> list[tuple[int, int, int]]:
    candidates = [name]
    if not name.startswith("b_"):
        candidates.append(f"b_{name}")
    if not name.startswith("m_"):
        candidates.append(f"m_{name}")

    for attr in candidates:
        if not hasattr(cc, attr):
            continue
        raw = getattr(cc, attr)
        if isinstance(raw, list) and raw:
            if isinstance(raw[0], str):
                return [_hex_to_rgb(v) for v in raw]
            if isinstance(raw[0], (list, tuple)):
                return [_rgb_to_uint8(v) for v in raw]

    raise ValueError(
        f"Unknown Colorcet colormap '{name}'. Tried: {', '.join(candidates)}"
    )


def _load_from_hex_or_txt(path: str) -> list[tuple[int, int, int]]:
    colors: list[tuple[int, int, int]] = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            item = line.strip()
            if not item or item.startswith("//") or item.startswith(";"):
                continue
            colors.append(_hex_to_rgb(item))
    return colors


def _load_from_csv(path: str) -> list[tuple[int, int, int]]:
    colors: list[tuple[int, int, int]] = []
    with open(path, "r", encoding="utf-8", newline="") as f:
        for row in csv.reader(f):
            if not row:
                continue
            if len(row) < 3:
                raise ValueError(f"CSV row must have at least 3 columns: {row}")
            colors.append(_rgb_to_uint8([float(row[0]), float(row[1]), float(row[2])]))
    return colors


def _load_from_json(path: str) -> list[tuple[int, int, int]]:
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    if isinstance(data, dict):
        if "colors" in data:
            data = data["colors"]
        elif "colormap" in data:
            data = data["colormap"]
        else:
            raise ValueError("JSON colormap dict must contain 'colors' or 'colormap'")

    if not isinstance(data, list):
        raise ValueError("JSON colormap must be a list or dict with a color list")

    colors: list[tuple[int, int, int]] = []
    for entry in data:
        if isinstance(entry, str):
            colors.append(_hex_to_rgb(entry))
        elif isinstance(entry, (list, tuple)):
            colors.append(_rgb_to_uint8(entry))
        else:
            raise ValueError("Unsupported JSON colormap entry format")
    return colors


def load_colormap(source: str = "glasbey_hv") -> list[tuple[int, int, int]]:
    """Load a colormap from Colorcet by name or from local files.

    Parameters
    ----------
    source:
        Either a Colorcet colormap name (e.g. "glasbey_hv" or "b_glasbey_hv")
        or a file path with extension .hex/.txt/.csv/.json.
    """
    if os.path.isfile(source):
        ext = os.path.splitext(source)[1].lower()
        if ext in {".hex", ".txt"}:
            colors = _load_from_hex_or_txt(source)
        elif ext == ".csv":
            colors = _load_from_csv(source)
        elif ext == ".json":
            colors = _load_from_json(source)
        else:
            raise ValueError(f"Unsupported colormap file extension: {ext}")
    else:
        colors = _load_from_colorcet(source)

    if not colors:
        raise ValueError("Loaded colormap is empty")
    return colors


_DEFAULT_CMAP: list[tuple[int, int, int]] = load_colormap("glasbey_hv")


def _resolve_mode(mode: str) -> str:
    mode = str(mode).strip().lower()
    if mode not in _MODES:
        raise ValueError(f"Invalid mode '{mode}', expected one of: {sorted(_MODES)}")
    if mode in {"round-robin", "rounding-robin"}:
        return "round-robin"
    return mode


def _resolve_index(label: int, n_labels: int | None, cmap_size: int, mode: str) -> int:
    if mode == "round-robin":
        return (label - 1) % cmap_size

    if n_labels is None:
        raise ValueError("n_labels is required when mode='stretch'")
    if n_labels <= 0:
        raise ValueError(f"n_labels must be > 0 for stretch mode, got {n_labels}")

    pos = min(max(label, 1), n_labels) - 1
    if n_labels == 1:
        return 0
    return int(round(pos * (cmap_size - 1) / (n_labels - 1)))


def _interpolate_color(
    label: int,
    n_labels: int,
    cmap: list[tuple[int, int, int]],
) -> tuple[int, int, int]:
    if n_labels <= 0:
        raise ValueError(f"n_labels must be > 0 for stretch mode, got {n_labels}")
    if len(cmap) == 1 or n_labels == 1:
        return cmap[0]

    pos = min(max(label, 1), n_labels) - 1
    scaled = pos * (len(cmap) - 1) / (n_labels - 1)
    lo = int(np.floor(scaled))
    hi = int(np.ceil(scaled))
    if lo == hi:
        return cmap[lo]

    t = scaled - lo
    c0 = np.asarray(cmap[lo], dtype=np.float64)
    c1 = np.asarray(cmap[hi], dtype=np.float64)
    blended = (1.0 - t) * c0 + t * c1
    return tuple(int(round(v)) for v in blended)


def label_rgb(
    label: int,
    n_labels: int | None = None,
    mode: str = "round-robin",
    cmap: list[tuple[int, int, int]] | None = None,
) -> tuple[int, int, int]:
    """Return a uint8 (R, G, B) tuple for *label*.

    Parameters
    ----------
    label:
        1-based cluster/class label.  Values ≤ 0 are treated as unlabeled
        and return white ``(255, 255, 255)``.
    n_labels:
        Total number of distinct labels. Required for mode='stretch'.
    mode:
        Mapping mode: 'round-robin' (default) or 'stretch'.
    cmap:
        Optional custom colormap as a list of (R, G, B) tuples.
    """
    if label <= 0:
        return _UNLABELED
    mode = _resolve_mode(mode)
    palette = _DEFAULT_CMAP if cmap is None else cmap
    if not palette:
        raise ValueError("Colormap palette must not be empty")
    if mode == "stretch":
        if n_labels is None:
            raise ValueError("n_labels is required when mode='stretch'")
        return _interpolate_color(label=label, n_labels=n_labels, cmap=palette)
    idx = _resolve_index(label=label, n_labels=n_labels, cmap_size=len(palette), mode=mode)
    return palette[idx]


def build_palette(
    n_labels: int,
    mode: str = "round-robin",
    cmap: list[tuple[int, int, int]] | None = None,
) -> list[tuple[int, int, int]]:
    """Return a list of ``n_labels`` distinct RGB colors (1-based labels).

    Index 0 in the returned list corresponds to label 1.

    Parameters
    ----------
    n_labels:
        Number of distinct labels needed.
    """
    mode = _resolve_mode(mode)
    palette = _DEFAULT_CMAP if cmap is None else cmap
    return [label_rgb(i + 1, n_labels=n_labels, mode=mode, cmap=palette) for i in range(n_labels)]


def labels_to_rgb_array(
    labels: list[int],
    n_labels: int | None = None,
    mode: str = "round-robin",
    cmap: list[tuple[int, int, int]] | None = None,
) -> np.ndarray:
    """Convert a list of integer labels to a float32 (N, 3) RGB array.

    Parameters
    ----------
    labels:
        Sequence of 1-based label integers (0 = unlabeled → white).

    Returns
    -------
    np.ndarray of shape (N, 3), dtype float32, values in [0, 255].
    """
    mode = _resolve_mode(mode)
    palette = _DEFAULT_CMAP if cmap is None else cmap

    # For stretch mode, infer n_labels from positive labels when absent.
    if mode == "stretch" and n_labels is None:
        positive = [int(v) for v in labels if int(v) > 0]
        n_labels = max(positive) if positive else 1

    out = np.empty((len(labels), 3), dtype=np.float32)
    for i, lbl in enumerate(labels):
        out[i] = label_rgb(lbl, n_labels=n_labels, mode=mode, cmap=palette)
    return out


def build_label_legend(
    labels: list[int] | None = None,
    n_labels: int | None = None,
    mode: str = "round-robin",
    cmap: list[tuple[int, int, int]] | None = None,
    include_unlabeled: bool = False,
) -> list[dict[str, int | str | list[int]]]:
    """Build a label-to-color legend for the active colormap configuration.

    Parameters
    ----------
    labels:
        Optional sequence of labels actually present in the output.
    n_labels:
        Optional upper bound on labels. If omitted, inferred from ``labels``.
    mode:
        Mapping mode: round-robin or stretch.
    cmap:
        Optional colormap as a list of (R, G, B) tuples.
    include_unlabeled:
        When true, add a label 0 entry mapped to white.
    """
    mode = _resolve_mode(mode)
    palette = _DEFAULT_CMAP if cmap is None else cmap
    if not palette:
        raise ValueError("Colormap palette must not be empty")

    if labels is not None:
        label_values = sorted({int(v) for v in labels if int(v) > 0})
        inferred_n = max(label_values) if label_values else 0
    else:
        label_values = []
        inferred_n = 0

    if n_labels is None:
        n_labels = inferred_n
    elif labels is None:
        label_values = list(range(1, n_labels + 1))

    if not label_values and n_labels:
        label_values = list(range(1, n_labels + 1))

    legend: list[dict[str, int | str | list[int]]] = []
    if include_unlabeled:
        legend.append({"label": 0, "hex": "#ffffff", "rgb": [255, 255, 255]})

    for label in label_values:
        rgb = label_rgb(label=label, n_labels=n_labels, mode=mode, cmap=palette)
        legend.append(
            {
                "label": int(label),
                "hex": f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}",
                "rgb": [int(rgb[0]), int(rgb[1]), int(rgb[2])],
            }
        )
    return legend


def export_label_legend(
    path: str,
    labels: list[int] | None = None,
    n_labels: int | None = None,
    mode: str = "round-robin",
    cmap: list[tuple[int, int, int]] | None = None,
    include_unlabeled: bool = False,
    colormap_source: str | None = None,
) -> str:
    """Export a label legend to .json, .csv, or .txt based on file extension."""
    legend = build_label_legend(
        labels=labels,
        n_labels=n_labels,
        mode=mode,
        cmap=cmap,
        include_unlabeled=include_unlabeled,
    )
    ext = os.path.splitext(path)[1].lower()

    if ext == ".json":
        payload = {
            "mode": _resolve_mode(mode),
            "colormap_source": colormap_source,
            "entries": legend,
        }
        with open(path, "w", encoding="utf-8") as f:
            json.dump(payload, f, indent=2)
            f.write("\n")
        return path

    if ext == ".csv":
        with open(path, "w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["label", "hex", "r", "g", "b"])
            for entry in legend:
                rgb = entry["rgb"]
                writer.writerow([entry["label"], entry["hex"], rgb[0], rgb[1], rgb[2]])
        return path

    if ext == ".txt":
        with open(path, "w", encoding="utf-8") as f:
            f.write(f"mode={_resolve_mode(mode)}\n")
            if colormap_source is not None:
                f.write(f"colormap_source={colormap_source}\n")
            for entry in legend:
                rgb = entry["rgb"]
                f.write(
                    f"label={entry['label']} hex={entry['hex']} rgb=({rgb[0]}, {rgb[1]}, {rgb[2]})\n"
                )
        return path

    raise ValueError("Legend export path must use .json, .csv, or .txt")
