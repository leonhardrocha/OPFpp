"""
opfppy.supervised — high-level wrappers for the supervised OPF workflow.

Typical usage::

    from opfppy.supervised import train_and_classify, learn_and_classify
    from opfppy.utils import load, split, accuracy

    data = load("../data/boat.dat")
    train, test = split(data, 0.5)

    acc = train_and_classify(train, test)
    print(f"Accuracy: {acc:.2%}")
"""

from __future__ import annotations

import opfpy
from opfpy import Subgraph


def train(sg_train: Subgraph) -> None:
    """Train a supervised OPF model in-place.

    Parameters
    ----------
    sg_train : Subgraph  Modified in-place (prototype nodes identified,
                         optimum-path forest built).
    """
    opfpy.OPF().train(sg_train)


def classify(sg_train: Subgraph, sg_test: Subgraph) -> None:
    """Classify ``sg_test`` using a previously trained ``sg_train``.

    Parameters
    ----------
    sg_train : Subgraph  Already trained.
    sg_test  : Subgraph  Node labels set in-place.
    """
    opfpy.OPF().classify(sg_train, sg_test)


def train_and_classify(sg_train: Subgraph, sg_test: Subgraph) -> float:
    """Train on *sg_train* and classify *sg_test*.

    Mirrors *example 1* (supervised OPF without learning).

    Parameters
    ----------
    sg_train : Subgraph
    sg_test  : Subgraph

    Returns
    -------
    float  Accuracy on *sg_test* in [0, 1].
    """
    clf = opfpy.OPF()
    clf.train(sg_train)
    clf.classify(sg_train, sg_test)
    return clf.accuracy(sg_test)


def learn_and_classify(
    sg_train: Subgraph,
    sg_eval: Subgraph,
    sg_test: Subgraph,
    n_iterations: int = 10,
) -> float:
    """Run iterative learning on *sg_train*/*sg_eval*, then classify *sg_test*.

    Mirrors *example 2* (supervised OPF with learning).

    Parameters
    ----------
    sg_train     : Subgraph
    sg_eval      : Subgraph
    sg_test      : Subgraph
    n_iterations : int  Maximum learning iterations (default 10).

    Returns
    -------
    float  Accuracy on *sg_test* in [0, 1].
    """
    clf = opfpy.OPF()
    clf.learn(sg_train, sg_eval, n_iterations)
    clf.classify(sg_train, sg_test)
    return clf.accuracy(sg_test)


def prune(
    sg_train: Subgraph,
    sg_eval: Subgraph,
    tolerance: float = 0.02,
) -> float:
    """Iteratively prune irrelevant training nodes.

    Mirrors the *opf_pruning* tool.

    Parameters
    ----------
    sg_train  : Subgraph  Modified in-place (nodes removed).
    sg_eval   : Subgraph  Used to guide which training nodes are relevant.
    tolerance : float     Maximum allowed per-iteration accuracy drop.
                          Pruning stops when the drop exceeds this threshold
                          or after 100 iterations.

    Returns
    -------
    float  Fraction of training nodes removed (0 = none removed).
    """
    return opfpy.OPF().pruning(sg_train, sg_eval, tolerance)
