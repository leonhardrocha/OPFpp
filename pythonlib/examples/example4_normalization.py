"""
example4_normalization.py — supervised OPF with feature normalization
(mirrors example4.sh: split → normalize → train → classify).

Usage (from the pythonlib/ directory):
    python example4_normalization.py [path/to/dataset.dat]

Default dataset: ../data/boat.dat
"""

import sys
import os

_PYTHONLIB_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)

from opf.utils import load, split, normalize, info
from opf.supervised import train_and_classify

DATA_FILE = os.path.join(os.path.dirname(__file__), "..", "..", "data", "boat.dat")


def main(data_path: str = DATA_FILE) -> None:
    print("Example 4 — Supervised OPF with Feature Normalization")
    print("=======================================================")

    data = load(data_path)
    print(f"Dataset: {data_path}")
    print(f"  {info(data)}")

    # Normalize the whole dataset before splitting
    normalize(data)
    print("  Features normalized (z-score).")

    train_sg, test_sg = split(data, 0.5)
    print(f"  Train: {train_sg.nnodes}  |  Test: {test_sg.nnodes}")

    acc = train_and_classify(train_sg, test_sg)
    print(f"  Accuracy: {acc:.2%}")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DATA_FILE
    main(path)
