"""
opfppy.utils — helper functions for loading, splitting, normalising, and
evaluating OPF subgraphs.  All I/O is delegated to the opfpy C++ extension.
"""

from __future__ import annotations

import struct

import opfpy
from opfpy import Subgraph
from opfppy.distance import DistanceMetric, DistanceSpec, resolve as _resolve_distance


# ---------------------------------------------------------------------------
# I/O helpers
# ---------------------------------------------------------------------------

def load(path: str) -> Subgraph:
    """Load a subgraph from an OPF binary dataset file (original format).

    Parameters
    ----------
    path : str
        Path to the ``.dat`` file produced by the LibOPF tools.

    Returns
    -------
    Subgraph
    """
    return Subgraph.from_original_file(path)


def read(path: str) -> Subgraph:
    """Read a subgraph from the OPF training binary format.

    Parameters
    ----------
    path : str

    Returns
    -------
    Subgraph
    """
    return opfpy.read_subgraph(path)


def write(path: str, sg: Subgraph) -> None:
    """Write a subgraph to the OPF training binary format.

    Parameters
    ----------
    path : str
    sg   : Subgraph
    """
    opfpy.write_subgraph(path, sg)


# ---------------------------------------------------------------------------
# Data manipulation
# ---------------------------------------------------------------------------

def split(sg: Subgraph, pct_first: float) -> tuple[Subgraph, Subgraph]:
    """Label-stratified split into two subgraphs.

    Parameters
    ----------
    sg        : Subgraph
    pct_first : float   Fraction of samples to place in the first partition.

    Returns
    -------
    (first, second) : tuple[Subgraph, Subgraph]
    """
    return opfpy.split_subgraph(sg, pct_first)


def merge(sg1: Subgraph, sg2: Subgraph) -> Subgraph:
    """Merge two subgraphs (same number of features) into one.

    Parameters
    ----------
    sg1, sg2 : Subgraph

    Returns
    -------
    Subgraph
    """
    return opfpy.merge_subgraphs(sg1, sg2)


def k_fold(sg: Subgraph, k: int) -> list[Subgraph]:
    """Stratified k-fold partition.

    Parameters
    ----------
    sg : Subgraph
    k  : int   Number of folds.

    Returns
    -------
    list[Subgraph]  Length *k*.
    """
    return opfpy.k_fold(sg, k)


def normalize(sg: Subgraph) -> None:
    """Z-score feature normalisation **in-place** (mean 0, std 1 per feature).

    Parameters
    ----------
    sg : Subgraph  Modified in-place.
    """
    opfpy.OPF().normalize(sg)


# ---------------------------------------------------------------------------
# Evaluation
# ---------------------------------------------------------------------------

def accuracy(sg: Subgraph) -> float:
    """Compute OPF accuracy from ``label`` vs ``truelabel`` on each node.

    Parameters
    ----------
    sg : Subgraph  Must have been classified first.

    Returns
    -------
    float  Value in [0, 1].
    """
    return opfpy.OPF().accuracy(sg)


def info(sg: Subgraph) -> dict:
    """Return a dict with basic subgraph metadata.

    Returns
    -------
    dict  Keys: ``nnodes``, ``nlabels``, ``nfeats``.
    """
    return opfpy.subgraph_info(sg)


# ---------------------------------------------------------------------------
# Distance matrix
# ---------------------------------------------------------------------------

def compute_distance_matrix(
    sg: Subgraph,
    distance: DistanceSpec = DistanceMetric.EUCLIDEAN,
) -> list[list[float]]:
    """Compute the pairwise distance matrix for a subgraph.

    Parameters
    ----------
    sg       : Subgraph
    distance : int | str | DistanceMetric
        Which distance function to use.  Accepts:
        * A ``DistanceMetric`` enum member (e.g. ``DistanceMetric.MANHATTAN``).
        * A name string, case-insensitive (e.g. ``"manhattan"``, ``"L1"``).
        * A legacy integer id (1–7).
        Defaults to ``DistanceMetric.EUCLIDEAN``.

    Returns
    -------
    list[list[float]]  Symmetric n×n matrix.
    """
    return opfpy.compute_distance_matrix(sg, _resolve_distance(distance))


def write_distance_matrix(matrix: list[list[float]], path: str) -> None:
    """Write a precomputed distance matrix to a binary file.

    Binary format: int32 n (number of nodes), then n×n float32 values row-major.
    This matches the format expected by opfpy.read_distance_matrix.

    Parameters
    ----------
    matrix : list[list[float]]
    path   : str
    """
    n = len(matrix)
    with open(path, 'wb') as f:
        f.write(struct.pack('<i', n))
        for row in matrix:
            f.write(struct.pack(f'<{n}f', *row))


def read_distance_matrix(path: str) -> list[list[float]]:
    """Read a precomputed distance matrix from a binary file.

    Parameters
    ----------
    path : str

    Returns
    -------
    list[list[float]]
    """
    return opfpy.read_distance_matrix(path)
