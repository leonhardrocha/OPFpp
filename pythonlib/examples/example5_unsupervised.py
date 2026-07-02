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

DATA_FILE = os.path.join(os.path.dirname(__file__), "..", "..", "data", "data1.dat")



def main(data_path: str = DATA_FILE) -> None:
    print("Example 5 — Unsupervised OPF Clustering + k-NN Classify")
    print("==========================================================")

    # Load dataset (unlabeled for clustering)
    sg = load(data_path)
    print(f"Dataset: {data_path}")
    print(f"  {info(sg)}")

    # Unsupervised clustering: bestk_min_cut finds optimal k, clusters the data
    clf = opfppy.OPF()
    # Typical k range for bestk_min_cut is 2 to 10 (can be tuned)
    clf.bestk_min_cut(sg, 2, 10)
    print("  Performed unsupervised clustering with best-k min-cut.")

    # Propagate cluster labels (assigns a unique label to each cluster/tree)
    opfppy.propagate_cluster_labels(sg)
    print("  Propagated cluster labels to all nodes.")

    # Optionally, print number of clusters found
    cluster_labels = {sg.get_node(i).label for i in range(sg.nnodes)}
    print(f"  Number of clusters found: {len(cluster_labels)}")

    # For demonstration, split into train/test and use k-NN classify
    train_sg, test_sg = split(sg, 0.5)
    clf.knn_classify(train_sg, test_sg)
    acc = accuracy(test_sg)
    print(f"  k-NN classification accuracy: {acc:.2%}")


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else DATA_FILE
    main(path)
