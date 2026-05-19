"""
opfppy.unsupervised — high-level wrappers for unsupervised and semi-supervised
OPF workflows.

Typical usage::

    from opfppy.unsupervised import cluster_and_propagate
    from opfppy.utils import load, split, accuracy

    data = load("../data/data1.dat")
    train, test = split(data, 0.8)

    cluster_and_propagate(train, k=100)
    clf = opfpy.OPF()
    clf.knn_classify(train, test)
    print("Accuracy:", accuracy(test))
"""

from __future__ import annotations

import opfpy
from typing import Sequence

from opfpy import KernelSubGraph, Subgraph


def split_subgraph_into_kernels(sg: Subgraph, slices: list[tuple[int, int]]) -> list[KernelSubGraph]:
    """Split a Subgraph into :class:`KernelSubGraph` partitions by (offset, size) slices.

    Delegates to the C++ ``opfpy.split_subgraph_into_kernels`` implementation.
    Each entry in `slices` is a (offset, size) tuple, and the kernel covers
    features [offset, offset+size). Out-of-bounds or zero-size slices are skipped.

    Parameters
    ----------
    sg : Subgraph
        Source graph.  ``sg.nfeats`` must be > 0.
    slices : list[tuple[int, int]]
        List of (offset, size) pairs for each kernel slice.

    Returns
    -------
    list[KernelSubGraph]
        List of :class:`KernelSubGraph` objects, one per valid slice.
    """
    return opfpy.split_subgraph_into_kernels(sg, slices)
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


def bestk_cluster_and_propagate(
    sg: Subgraph,
    kmin: int = 2,
    kmax: int = 10,
    weighted_kernel: bool = False,
    kernels: Sequence[KernelSubGraph] | None = None,
    kernel_weights: list[float] | None = None,
) -> None:
    """Best-k unsupervised clustering followed by label propagation.

    When ``weighted_kernel=True`` the PDF for each arc (p, q) is computed as::

        sum_i( w_i * log(K_i(p, q)) )

    where ``K_i(p, q) = exp(-dist_i(p, q) / K)`` is the Gaussian kernel
    evaluated on the i-th feature slice and ``dist_i`` is the Euclidean
    distance restricted to that slice.

    Parameters
    ----------
    sg : Subgraph
        Input subgraph. Modified in-place.
    kmin, kmax : int
        k-range for best-k min-cut search.
    weighted_kernel : bool
        Enable kernel-weighted PDF. When True, ``kernels`` or an
        auto-split is used to define the feature partitions.
    kernels : list[Subgraph] | None
        Explicit kernel Subgraphs produced by :func:`split_subgraph_into_kernels`.
        Each kernel's ``nfeats`` defines the feature-slice size for that kernel.
        If None and ``weighted_kernel=True``, a per-feature kernel split is
        used (one kernel per feature dimension, uniform weights).
    kernel_weights : list[float] | None
        Per-kernel scalar weights.  Must match ``len(kernels)`` when supplied.
        Defaults to uniform 1/n weights.
    """
    if weighted_kernel:
        if kernels is None:
            # Auto-split: one slice per feature dimension.
            nfeats = sg.nfeats
            slices = [(i, 1) for i in range(nfeats)]
            kernels = split_subgraph_into_kernels(sg, slices)
        sizes = [k.nfeats for k in kernels]
        if kernel_weights is None:
            uniform_w = 1.0 / float(max(len(sizes), 1))
            kernel_weights = [uniform_w] * len(sizes)
        sg.kernel_feature_sizes = sizes
        sg.kernel_weights = kernel_weights

    clf = opfpy.OPF()
    clf.bestk_min_cut(sg, kmin, kmax)
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
