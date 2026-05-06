"""
example2_learning.py — supervised OPF with learning (mirrors example2.sh).

Usage (from the pythonlib/ directory):
    python example2_learning.py [path/to/dataset.dat]

Default dataset: ../data/boat.dat
"""

import sys
import os

_PYTHONLIB_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)

from opfppy.utils import load, split, info
from opfppy.supervised import learn_and_classify

DATA_FILE = os.path.join(os.path.dirname(__file__), "..", "..", "data", "boat.dat")


def main(data_path: str = DATA_FILE) -> None:
    print("Example 2 — Supervised OPF with Learning")
    print("==========================================")

    data = load(data_path)
    print(f"Dataset: {data_path}")
    print(f"  {info(data)}")

    # Split 30% train / 20% eval / 50% test (two-pass split)
    train_sg, rest = split(data, 0.3)
    eval_sg, test_sg = split(rest, 0.4)   # 0.4 of 0.7 ≈ 0.28 → eval ~20%
    print(f"  Train: {train_sg.nnodes}  |  Eval: {eval_sg.nnodes}  |  Test: {test_sg.nnodes}")

    acc = learn_and_classify(train_sg, eval_sg, test_sg, n_iterations=10)
    print(f"  Accuracy: {acc:.2%}")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DATA_FILE
    main(path)
