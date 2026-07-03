"""
example7_ply_subgraph.py — build an OPF subgraph from a Gaussian-splat PLY.

Usage (from the pythonlib/ directory):
    python examples/example7_ply_subgraph.py [path/to/sample.ply] [full|compact]

Default dataset:
    ../../tools/bridge-server/sample.ply
"""

import os
import sys

# Ensure pythonlib package root is importable when running from examples/.
_PYTHONLIB_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)

from opfppy.utils import load_ply

DEFAULT_PLY = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..", "tools", "bridge-server", "sample.ply")
)


def main(ply_path: str = DEFAULT_PLY, feature_profile: str = "full") -> None:
    print("Example 7 — PLY Gaussian -> Subgraph")
    print("=====================================")
    print(f"PLY input: {ply_path}")
    print(f"Feature profile: {feature_profile}")

    subgraph, meta = load_ply(ply_path, feature_profile=feature_profile)

    print(f"Subgraph nodes: {subgraph.nnodes}")
    print(f"Subgraph features per node: {subgraph.nfeats}")
    print(f"Detected scene properties: {len(meta['scene_properties'])}")

    sample_codes = ["f_dc_0", "f_rest_0", "f_rest_3", "f_rest_44"]
    print("SH code preview:")
    for name in sample_codes:
        code = meta["sh_codes"][name]
        print(f"  {name:<10} -> {code:3d} ({code:08b})")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PLY
    profile = sys.argv[2] if len(sys.argv) > 2 else "full"
    main(path, profile)
