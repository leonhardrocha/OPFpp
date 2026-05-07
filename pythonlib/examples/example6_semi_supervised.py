"""
example6_semi_supervised.py — semi-supervised OPF learning (mirrors example6.sh).

Usage (from the pythonlib/ directory):
    python example6_semi_supervised.py [path/to/dataset.dat]

Default dataset: ../data/saturn.dat
"""

import sys
import os

_PYTHONLIB_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)

from opfppy.utils import load, split, accuracy, info
from opfppy.unsupervised import semi_supervised
from opfppy.supervised import classify as opf_classify

DATA_FILE = os.path.join(os.path.dirname(__file__), "..", "..", "data", "saturn.dat")


def main(data_path: str = DATA_FILE) -> None:
    print("Example 6 — Semi-Supervised OPF")
    print("==================================")

    data = load(data_path)
    print(f"Dataset: {data_path}")
    print(f"  {info(data)}")

    # Step 1: split 60 % Z1 (will be split further) + 20 % eval + 20 % test
    z1, rest = split(data, 0.6)
    # eval not needed for the basic semi-supervised path (no iterative learning)
    _, test_sg = split(rest, 0.5)

    # Step 2: from Z1 carve out 40 % labeled / 60 % unlabeled
    labeled_sg, unlabeled_sg = split(z1, 0.4)
    print(f"  Labeled: {labeled_sg.nnodes}  |  Unlabeled: {unlabeled_sg.nnodes}"
          f"  |  Test: {test_sg.nnodes}")

    # Step 3: semi-supervised training (returns merged trained subgraph)
    merged_sg = semi_supervised(labeled_sg, unlabeled_sg)
    print(f"  Merged training graph: {merged_sg.nnodes} nodes")

    # Step 4: classify the hold-out test set
    opf_classify(merged_sg, test_sg)
    acc = accuracy(test_sg)
    print(f"  Accuracy: {acc:.2%}")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DATA_FILE
    main(path)
