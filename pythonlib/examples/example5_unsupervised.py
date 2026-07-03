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
import opfppy
from opfppy.unsupervised import bestk_cluster_and_propagate

DATA_FILE = os.path.join(os.path.dirname(__file__), "..", "..", "data", "data1.dat")


def _run_workflow(data_path: str, weighted_kernel: bool) -> float:
    mode = "weighted" if weighted_kernel else "original"

    # Load dataset (unlabeled for clustering)
    sg = load(data_path)
    print(f"\nMode: {mode}")
    print(f"  Dataset: {data_path}")
    print(f"  {info(sg)}")

    bestk_cluster_and_propagate(
        sg,
        kmin=2,
        kmax=10,
        weighted_kernel=weighted_kernel,
    )
    print("  Performed unsupervised clustering with best-k min-cut.")
    print("  Propagated cluster labels to all nodes.")

    cluster_labels = {sg.get_node(i).label for i in range(sg.nnodes)}
    print(f"  Number of clusters found: {len(cluster_labels)}")

    train_sg, test_sg = split(sg, 0.5)
    clf = opfppy.OPF()
    clf.knn_classify(train_sg, test_sg)
    acc = accuracy(test_sg)
    print(f"  k-NN classification accuracy: {acc:.2%}")
    return acc



def main(data_path: str = DATA_FILE) -> None:
    print("Example 5 — Unsupervised OPF Clustering + k-NN Classify")
    print("==========================================================")

    original_acc = _run_workflow(data_path, weighted_kernel=False)
    weighted_acc = _run_workflow(data_path, weighted_kernel=True)
    delta = weighted_acc - original_acc

    print("\nComparison")
    print("----------")
    print(f"  Original accuracy: {original_acc:.2%}")
    print(f"  Weighted accuracy: {weighted_acc:.2%}")
    print(f"  Delta (weighted - original): {delta:+.2%}")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DATA_FILE
    main(path)
