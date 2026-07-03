"""
example8_splat_subgraph.py — advanced PLY loading with SplatSubGraph class.

Demonstrates loading a Gaussian-splat PLY into a SplatSubGraph with metadata,
comparing full vs compact profiles, and exporting to OPF format.

Usage (from the pythonlib/ directory):
    python examples/example8_splat_subgraph.py [path/to/sample.ply]

Default dataset:
    ../../tools/bridge-server/sample.ply
"""

import os
import sys

# Ensure pythonlib package root is importable when running from examples/.
_PYTHONLIB_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)

from opfppy.ply_adapter import SplatSubGraph

DEFAULT_PLY = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..", "tools", "bridge-server", "sample.ply")
)


def main(ply_path: str = DEFAULT_PLY) -> None:
    print("Example 8 — SplatSubGraph: Advanced PLY loading")
    print("=" * 50)

    # Load both profiles
    print("\n1. Loading full profile...")
    sg_full = SplatSubGraph.from_ply_file(ply_path, feature_profile="full")
    print(sg_full.summary())

    print("\n2. Loading compact profile...")
    sg_compact = SplatSubGraph.from_ply_file(ply_path, feature_profile="compact")
    print(sg_compact.summary())

    # Compare profiles
    print("\n3. Profile comparison:")
    print(f"  Memory savings: {(1 - sg_compact.nfeats/sg_full.nfeats)*100:.1f}%")
    print(f"  Full features:    {sg_full.nfeats}")
    print(f"  Compact features: {sg_compact.nfeats}")

    # Display SH code sample
    print("\n4. SH harmonic encoding sample:")
    sample_names = ["f_dc_0", "f_rest_0", "f_rest_15", "f_rest_44"]
    for name in sample_names:
        if name in sg_full.metadata["sh_codes"]:
            code = sg_full.metadata["sh_codes"][name]
            print(f"  {name:<10} -> decimal {code:3d} -> binary {code:08b}")

    # Optional: save to OPF
    print("\n5. Exporting to OPF binary format...")
    output_opf = os.path.join(os.path.dirname(ply_path), "sample_full.opf")
    sg_full.to_opf_model(output_opf)
    print(f"  Saved: {output_opf}")

    print("\nDone!")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PLY
    main(path)
