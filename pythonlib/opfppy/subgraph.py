"""
opfppy.subgraph — Python shim for opfpy.Subgraph.

Follows the same pattern as opfppy.distance: this module wraps the raw C++
``opfpy.Subgraph`` in a friendlier Python class.  The ``wrap()`` factory
(analogous to ``resolve()``) converts any bare ``opfpy.Subgraph`` returned
by C-level code into an ``opfppy.Subgraph``.

Usage
-----
>>> from opfppy.subgraph import Subgraph
>>> sg = Subgraph.from_original_file("data.dat")
>>> print(sg)           # pretty repr

Extending
---------
Subclass :class:`Subgraph` to add domain-specific attributes; all
pybind11 properties (nnodes, nfeats, nlabels, bestk, K, df, mindens,
maxdens) and methods (get_node, add_node, write_model, etc.) are inherited.
Call :meth:`register` to record the subclass for domain lookup.
"""

from __future__ import annotations

import opfpy as _opfpy

from opfppy.node import _fmt_seq


# ---------------------------------------------------------------------------
# Python shim class
# ---------------------------------------------------------------------------

class Subgraph(_opfpy.Subgraph):
    """Python-level wrapper around ``opfpy.Subgraph``.

    Inherits every pybind11 property and method from the C++ binding and
    adds a pretty ``__repr__``.  Factory class-methods mirror the static
    ``opfpy.Subgraph`` factories but return :class:`Subgraph` instances.
    """

    # ------------------------------------------------------------------
    # Factory methods — analogous to distance.resolve()
    # ------------------------------------------------------------------

    @classmethod
    def from_original_file(cls, filename: str) -> "Subgraph":
        """Load from the original LibOPF binary format.

        Parameters
        ----------
        filename : str  Path to a ``.dat`` file.

        Returns
        -------
        Subgraph
        """
        sg = _opfpy.Subgraph.from_original_file(filename)
        sg.__class__ = cls
        return sg

    @classmethod
    def read_model(cls, filename: str) -> "Subgraph":
        """Load from the OPF model binary format.

        Parameters
        ----------
        filename : str

        Returns
        -------
        Subgraph
        """
        sg = _opfpy.Subgraph.read_model(filename)
        sg.__class__ = cls
        return sg

    @classmethod
    def wrap(cls, sg: _opfpy.Subgraph) -> "Subgraph":
        """Not supported: pybind11 C-extension types block ``__class__`` reassignment.

        Use the factory class-methods (:meth:`from_original_file`,
        :meth:`read_model`) which return :class:`Subgraph` instances directly.
        """
        raise NotImplementedError(
            "Cannot promote opfpy.Subgraph to opfppy.Subgraph via __class__ assignment. "
            "Use Subgraph.from_original_file() or Subgraph.read_model() instead."
        )

    # ------------------------------------------------------------------
    # Pretty repr
    # ------------------------------------------------------------------

    def __repr__(self) -> str:  # noqa: D105
        nnodes = self.nnodes
        header = (
            "Subgraph("
            f"nnodes={nnodes}, nfeats={self.nfeats}, nlabels={self.nlabels}, "
            f"bestk={self.bestk}, K={self.K}, df={self.df:.6g}, "
            f"mindens={self.mindens:.6g}, maxdens={self.maxdens:.6g}"
        )

        if nnodes == 0:
            return header + ")"

        indices = list(range(min(3, nnodes)))
        if nnodes > 5:
            indices.append(-1)  # sentinel for "..."
        start_tail = max(3, nnodes - 2)
        for i in range(start_tail, nnodes):
            if i not in indices:
                indices.append(i)

        lines = [header + ","]
        for idx in indices:
            if idx == -1:
                lines.append("  ...")
            else:
                node = self.get_node(idx)
                # Format inline without mutating __class__ (pybind11 blocks it)
                node_text = (
                    "Node("
                    f"label={node.label}, truelabel={node.truelabel}, "
                    f"pathval={node.pathval:.6g}, dens={node.dens:.6g}, radius={node.radius:.6g}, "
                    f"root={node.root}, pred={node.pred}, position={node.position}, "
                    f"status={node.status}, relevant={node.relevant}, nplatadj={node.nplatadj}, "
                    f"feat={_fmt_seq(node.feat)}, adj={_fmt_seq(node.adj, precision=0)}"
                    ")"
                )
                lines.append(f"  [{idx}] {node_text}")
        lines.append(")")
        return "\n".join(lines)

    # ------------------------------------------------------------------
    # Registry — analogous to distance.register()
    # ------------------------------------------------------------------

    _registry: dict[str, type] = {}

    @classmethod
    def register(cls, name: str, subclass: type) -> None:
        """Register *subclass* under *name* for domain-specific wrapping.

        Parameters
        ----------
        name     : str   Logical name for the subclass.
        subclass : type  Must be a subclass of :class:`Subgraph`.
        """
        if not issubclass(subclass, cls):
            raise TypeError(f"{subclass!r} is not a subclass of Subgraph")
        cls._registry[name] = subclass
