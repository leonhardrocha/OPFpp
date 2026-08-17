"""
example3_precomputed_distances.py — supervised OPF with precomputed distances
(mirrors example3.sh).

Computes the Manhattan distance matrix, writes it to a temp file, reads it
back, then runs train + classify using standard opfpy.

Usage (from the pythonlib/ directory):
    python example3_precomputed_distances.py [path/to/dataset.dat]

Default dataset: ../data/cone-torus.dat
"""

import sys
import os
import tempfile

_PYTHONLIB_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)

from opfppy.utils import load, split, info, accuracy
from opfppy.utils import compute_distance_matrix, write_distance_matrix, read_distance_matrix
from opfppy.supervised import train_and_classify

DATA_FILE = os.path.join(os.path.dirname(__file__), "..", "..", "data", "cone-torus.dat")

DISTANCE_MANHATTAN = 3


def main(data_path: str = DATA_FILE) -> None:
    print("Example 3 — Supervised OPF with Precomputed Distances")
    print("=======================================================")

    data = load(data_path)
    print(f"Dataset: {data_path}")
    print(f"  {info(data)}")

    # 1. Compute Manhattan distance matrix and write/read roundtrip
    print("  Computing distance matrix (Manhattan)...")
    matrix = compute_distance_matrix(data, DISTANCE_MANHATTAN)

    with tempfile.NamedTemporaryFile(suffix=".dist", delete=False) as f:
        dist_path = f.name
    try:
        write_distance_matrix(matrix, dist_path)
        matrix2 = read_distance_matrix(dist_path)
        assert len(matrix2) == len(matrix), "Roundtrip row count mismatch"
        print(f"  Distance matrix written and read back ({len(matrix)}×{len(matrix[0])}).")
    finally:
        os.unlink(dist_path)

    # 2. Standard supervised train/classify (distance matrix not yet wired
    #    into OPF.hpp for on-the-fly lookup; this example validates the I/O path)
    train_sg, test_sg = split(data, 0.5)
    print(f"  Train: {train_sg.nnodes}  |  Test: {test_sg.nnodes}")
    acc = train_and_classify(train_sg, test_sg)
    print(f"  Accuracy: {acc:.2%}")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DATA_FILE
    main(path)
