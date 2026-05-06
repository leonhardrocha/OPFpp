"""Pretty repr support for pybind-backed opfpy classes.

This module patches ``opfpy.Node``, ``opfpy.Subgraph``, and ``opfpy.OPF``
``__repr__`` methods at import time. It is intentionally best-effort:
if ``opfpy`` is not importable yet, patching is skipped.
"""

from __future__ import annotations


def _fmt_seq(values, *, head: int = 3, tail: int = 2, precision: int = 4) -> str:
    """Format a sequence with head/tail truncation and element count."""
    items = list(values)
    n = len(items)

    def _fmt(v):
        if isinstance(v, float):
            return f"{v:.{precision}g}"
        return str(v)

    if n <= head + tail + 1:
        rendered = ", ".join(_fmt(v) for v in items)
    else:
        left = ", ".join(_fmt(v) for v in items[:head])
        right = ", ".join(_fmt(v) for v in items[-tail:])
        rendered = f"{left}, ..., {right}"

    return f"[{rendered}] ({n})"


def _patch() -> None:
    try:
        import opfpy
    except Exception:
        return

    def _node_repr(self) -> str:
        return (
            "Node("
            f"label={self.label}, truelabel={self.truelabel}, "
            f"pathval={self.pathval:.6g}, dens={self.dens:.6g}, radius={self.radius:.6g}, "
            f"root={self.root}, pred={self.pred}, position={self.position}, "
            f"status={self.status}, relevant={self.relevant}, nplatadj={self.nplatadj}, "
            f"feat={_fmt_seq(self.feat)}, adj={_fmt_seq(self.adj, precision=0)}"
            ")"
        )

    def _subgraph_repr(self) -> str:
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
            indices += [-1]
        start_tail = max(3, nnodes - 2)
        for i in range(start_tail, nnodes):
            if i not in indices:
                indices.append(i)

        lines = [header + ","]
        for idx in indices:
            if idx == -1:
                lines.append("  ...")
            else:
                lines.append(f"  [{idx}] {self.get_node(idx)!r}")
        lines.append(")")
        return "\n".join(lines)

    def _opf_repr(self) -> str:
        return "OPF()"

    opfpy.Node.__repr__ = _node_repr
    opfpy.Subgraph.__repr__ = _subgraph_repr
    opfpy.OPF.__repr__ = _opf_repr


_patch()
