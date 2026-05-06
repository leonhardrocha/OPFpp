"""
opf.unsupervised — high-level wrappers for unsupervised and semi-supervised
OPF workflows.

Typical usage::

    from opf.unsupervised import cluster_and_propagate
    from opf.utils import load, split, accuracy

    data = load("../data/data1.dat")
    train, test = split(data, 0.8)

    cluster_and_propagate(train, k=100)
    clf = opfpy.OPF()
    clf.knn_classify(train, test)
    print("Accuracy:", accuracy(test))
"""

from __future__ import annotations

import opfpy
from opfpy import Subgraph


def cluster_and_propagate(sg: Subgraph, k: int) -> None:
    """Unsupervised OPF clustering followed by label propagation.

    Mirrors *example 5* (unsupervised OPF).

    Requires that ``sg`` has adjacency lists and density values already
    computed (i.e. the subgraph was produced by ``opf_cluster`` or the
    C++ ``clustering()`` path that builds arcs internally).

    For the Python path the caller is responsible for building the knn
    graph, PDF, and calling :func:`cluster`.

    Parameters
    ----------
    sg : Subgraph  Modified in-place.
    k  : int       Not used by :func:`cluster` directly — keep for API parity;
                   the caller should set ``sg.bestk = k`` before calling this.
    """
    sg.bestk = k
    clf = opfpy.OPF()
    clf.cluster(sg)
    opfpy.propagate_cluster_labels(sg)


def knn_classify(sg_train: Subgraph, sg_test: Subgraph) -> None:
    """k-NN OPF classification using per-node radius stored in ``sg_train``.

    Parameters
    ----------
    sg_train : Subgraph  Trained (cluster model).
    sg_test  : Subgraph  Labels set in-place.
    """
    opfpy.OPF().knn_classify(sg_train, sg_test)


def semi_supervised(
    sg_labeled: Subgraph,
    sg_unlabeled: Subgraph,
    sg_eval: Subgraph | None = None,
) -> Subgraph:
    """Semi-supervised OPF learning.

    Mirrors *example 6* (semi-supervised OPF).

    Parameters
    ----------
    sg_labeled   : Subgraph  Nodes with known labels.
    sg_unlabeled : Subgraph  Nodes whose labels are to be inferred.
    sg_eval      : Subgraph | None  Optional evaluation set for iterative
                   learning refinement.

    Returns
    -------
    Subgraph  Merged and trained subgraph containing both labeled and
              unlabeled nodes with inferred labels.
    """
    clf = opfpy.OPF()
    return clf.semi_supervised(sg_labeled, sg_unlabeled, sg_eval)
