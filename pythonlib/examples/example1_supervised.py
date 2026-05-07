"""
example1_supervised.py — supervised OPF without learning (mirrors example1.sh).

Usage (from the pythonlib/ directory):
    python example1_supervised.py [path/to/dataset.dat]

Default dataset: ../data/boat.dat
"""

import sys
import os

# Ensure pythonlib package root is importable when running from examples/.
_PYTHONLIB_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)

from opfppy.utils import load, split, accuracy, info
from opfppy.supervised import train_and_classify

DATA_FILE = os.path.join(os.path.dirname(__file__), "..", "data", "boat.dat")


def main(data_path: str = DATA_FILE) -> None:
    print("Example 1 — Supervised OPF (no learning)")
    print("==========================================")

    # 1. Load dataset
    data = load(data_path)
    print(f"Dataset: {data_path}")
    print(f"  {info(data)}")

    # 2. Split 50 / 50
    train_sg, test_sg = split(data, 0.5)
    print(f"  Train nodes: {train_sg.nnodes}  |  Test nodes: {test_sg.nnodes}")

    # 3. Train and classify
    acc = train_and_classify(train_sg, test_sg)
    print(f"  Accuracy: {acc:.2%}")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DATA_FILE
    main(path)
