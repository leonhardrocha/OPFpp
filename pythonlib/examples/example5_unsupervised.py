"""
example5_unsupervised.py — unsupervised OPF clustering with k-NN classify
(mirrors example5.sh).

Usage (from the pythonlib/ directory):
    python example5_unsupervised.py [path/to/dataset.dat]

Default dataset: ../data/data1.dat
"""

import sys
import os

_PYTHONLIB_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)

from opfppy.utils import load, split, accuracy, info
import opfpy

DATA_FILE = os.path.join(os.path.dirname(__file__), "..", "..", "data", "data1.dat")

# Nearest-neighbour k for the knn graph / pdf
K = 5


def _build_knn_and_pdf(sg: opfpy.Subgraph, k: int) -> None:
    """Set uniform radius so knn_classify can work without full BestKMinCut."""
    # Compute per-node radius as the k-th nearest-neighbour distance.
    n = sg.nnodes
    for i in range(n):
        fi = sg.get_node(i).feat
        dists = sorted(
            opfpy.eucl_dist(fi, sg.get_node(j).feat)
            for j in range(n) if j != i
        )
        sg.get_node(i).radius = dists[min(k - 1, len(dists) - 1)]


def main(data_path: str = DATA_FILE) -> None:
    print("Example 5 — Unsupervised OPF Clustering + k-NN Classify")
    print("==========================================================")

    data = load(data_path)
    print(f"Dataset: {data_path}")
    print(f"  {info(data)}")

    train_sg, test_sg = split(data, 0.8)
    print(f"  Train: {train_sg.nnodes}  |  Test: {test_sg.nnodes}")

    # Build adjacency / radius so knn_classify can run
    _build_knn_and_pdf(train_sg, K)

    # Propagate the truelabel of each root to its tree members so the
    # cluster model can be used as a classifier.
    opfpy.propagate_cluster_labels(train_sg)

    # k-NN classify test set
    clf = opfpy.OPF()
    clf.knn_classify(train_sg, test_sg)

    acc = clf.accuracy(test_sg)
    print(f"  Accuracy: {acc:.2%}")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DATA_FILE
    main(path)
