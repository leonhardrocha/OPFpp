"""
opfppy.ply_adapter — convert Gaussian Splatting PLY data into OPF subgraphs.

This module reads a PLY file using ``plyfile``, validates the expected Gaussian
Splat schema, and creates an ``opfppy.Subgraph`` where each vertex/splat is one
node with a feature vector.
"""

from __future__ import annotations

from typing import Any

from plyfile import PlyData

from opfppy.subgraph import Subgraph
from opfppy.node import Node

from numbers import Number

_EXCLUDE_NODE_FIELDS = {"feat", "adj"}

_REQUIRED_PROPERTIES = [
    "x", "y", "z",
    "nx", "ny", "nz",
    "f_dc_0", "f_dc_1", "f_dc_2",
    *[f"f_rest_{i}" for i in range(45)],
    "opacity",
    "scale_0", "scale_1", "scale_2",
    "rot_0", "rot_1", "rot_2", "rot_3",
]

_COMPACT_FEATURES = [
    "x", "y", "z",
    "f_dc_0", "f_dc_1", "f_dc_2",
    "opacity",
    "scale_0", "scale_1", "scale_2",
    "rot_0", "rot_1", "rot_2", "rot_3",
]


def encode_sh_params(l: int, m: int) -> int:
    """Pack SH ``(l, m)`` into one byte: 3 bits for ``l`` and 5 bits for ``m+16``."""
    if not 0 <= l <= 7:
        raise ValueError(f"l out of range [0, 7]: {l}")
    if not -16 <= m <= 15:
        raise ValueError(f"m out of range [-16, 15]: {m}")
    return ((l & 0x07) << 5) | ((m + 16) & 0x1F)


def decode_sh_params(value: int) -> tuple[int, int]:
    """Unpack one-byte SH code back into ``(l, m)`` using the offset convention."""
    if not 0 <= value <= 255:
        raise ValueError(f"byte value out of range [0, 255]: {value}")
    l = (value >> 5) & 0x07
    m = (value & 0x1F) - 16
    return l, m


def _lm_from_sh_property(name: str) -> tuple[int, int]:
    if name.startswith("f_dc_"):
        return 0, 0

    if not name.startswith("f_rest_"):
        raise ValueError(f"Unsupported SH property: {name}")

    rest_idx = int(name.split("_", 2)[2])
    if not 0 <= rest_idx <= 44:
        raise ValueError(f"f_rest index out of range [0, 44]: {rest_idx}")

    # Gaussian Splatting convention: 45 f_rest entries are 15 SH terms x 3 channels.
    coeff_idx = (rest_idx // 3) + 1
    l = int(coeff_idx ** 0.5)
    m = coeff_idx - (l * l) - l
    return l, m


def _validate_schema(property_names: list[str]) -> None:
    missing = [name for name in _REQUIRED_PROPERTIES if name not in property_names]
    if missing:
        raise ValueError(
            "PLY schema missing required Gaussian properties: "
            + ", ".join(missing)
        )


def _resolve_features(property_names: list[str], feature_profile: str) -> list[str]:
    if feature_profile == "full":
        return [name for name in _REQUIRED_PROPERTIES if name in property_names]
    if feature_profile == "compact":
        return [name for name in _COMPACT_FEATURES if name in property_names]
    raise ValueError("feature_profile must be 'full' or 'compact'")


def _sh_codes() -> dict[str, int]:
    codes: dict[str, int] = {}
    for name in ["f_dc_0", "f_dc_1", "f_dc_2", *[f"f_rest_{i}" for i in range(45)]]:
        l, m = _lm_from_sh_property(name)
        codes[name] = encode_sh_params(l, m)
    return codes


def subgraph_from_ply_file(path: str, feature_profile: str = "full") -> tuple[Subgraph, dict[str, Any]]:
    """Load a Gaussian-splat PLY and return ``(Subgraph, metadata)``.

    Parameters
    ----------
    path : str
        Path to the PLY file.
    feature_profile : str
        ``'full'`` includes all Gaussian properties.
        ``'compact'`` keeps a reduced subset for lower memory usage.
    """
    ply = PlyData.read(path)
    vertex = ply["vertex"]
    vertices = vertex.data

    property_names = list(vertices.dtype.names or [])
    _validate_schema(property_names)
    feature_names = _resolve_features(property_names, feature_profile)

    sg = Subgraph(len(vertices))
    sg.nfeats = len(feature_names)
    sg.nlabels = 1

    for i in range(len(vertices)):
        row = vertices[i]
        node = sg.get_node(i)
        node.feat = [float(row[name]) for name in feature_names]
        node.position = i
        node.label = 1
        node.truelabel = 1
        node.pathval = 0.0
        node.pred = -1
        node.status = 0
        node.relevant = 0
        node.root = i

    metadata: dict[str, Any] = {
        "source": path,
        "nnodes": int(sg.nnodes),
        "nfeats": int(sg.nfeats),        
        "feature_profile": feature_profile,
        "feature_names": feature_names,
        "scene_properties": property_names,
        "sh_codes": _sh_codes(),
        "packing": "byte=(l<<5)|((m+16)&31)",
    }
    return sg, metadata


def node_scalar_members(node : Node) -> dict[str, str]:
    import numpy as np
    out: dict[str, str] = {}
    for name in dir(node):
        if name.startswith("_") or name in _EXCLUDE_NODE_FIELDS:
            continue
        try:
            value = getattr(node, name)
        except Exception:
            continue
        if callable(value):
            continue
        # Map Python type to numpy dtype string
        if isinstance(value, bool):
            out[name] = 'b1'
        elif isinstance(value, int):
            out[name] = 'i4'
        elif isinstance(value, float):
            out[name] = 'f4'
        elif isinstance(value, str):
            # Use variable-length unicode string
            out[name] = 'U'
        elif value is None:
            out[name] = 'O'
    return out


def write_subgraph_to_ply_file(sg: Subgraph, path: str, metadata: dict[str, Any] | None = None, feature_profile: str = "full") -> None:
    """Export a Subgraph to a PLY file with the given feature profile and label property.

    Parameters
    ----------
    sg : Subgraph
        The subgraph to export.
    path : str
        Path to write the PLY file.
    metadata : dict[str, Any] or None
        Metadata dictionary (from from_ply_file or SplatSubGraph), used to determine feature names if available.
    feature_profile : str
        'full' or 'compact'.
    """
    import numpy as np
    from plyfile import PlyElement, PlyData

    # Determine feature names to export
    feature_names = None
    node_members = {}
    if metadata and "feature_names" in metadata and metadata.get("feature_profile") == feature_profile:
        feature_names = list(metadata["feature_names"])
    if feature_names is None:
        if "full" in feature_profile:
            feature_names = list(_REQUIRED_PROPERTIES)
        elif "compact" in feature_profile:
            feature_names = list(_COMPACT_FEATURES)    
        else:
            raise ValueError("feature_profile must be 'full' or 'compact'")    
    if "+" in feature_profile:
        # If profile is not explicitly in metadata, but matches a known set, use that set:
        for member, dtype in node_scalar_members(sg.get_node(0)).items():
            if member in feature_profile:
                node_members[member] = dtype

    # Build dtype for numpy structured array: all features as float32, label as int32
    dtype = [(name, 'f4') for name in feature_names]
    for member, type in node_members.items():
        dtype.append((member, type))
    arr = np.empty(sg.nnodes, dtype=dtype)

    for i in range(sg.nnodes):
        node = sg.get_node(i)
        feat_vals = list(node.feat)[:len(feature_names)]
        if len(feat_vals) < len(feature_names):
            feat_vals += [0.0] * (len(feature_names) - len(feat_vals))
        for j, name in enumerate(feature_names):
            arr[name][i] = feat_vals[j]        
        for member in node_members:
            arr[member][i] = getattr(node, member, 0)

    ply_el = PlyElement.describe(arr, 'vertex')
    PlyData([ply_el], text=False).write(path)


class SplatSubGraph(Subgraph):
    """Python-level wrapper around a Gaussian-splat Subgraph with persistent metadata.

    Inherits all pybind11 properties and methods from ``opfppy.Subgraph`` and adds:
    - Metadata dictionary from PLY source (feature names, scene properties, SH codes)
    - Import/export methods for PLY and OPF formats
    - Pretty repr with metadata summary
    """

    def __init__(self, nnodes: int = 0):
        """Create an empty SplatSubGraph."""
        super().__init__(nnodes)
        self.metadata: dict[str, Any] = {}

    @classmethod
    def from_ply_file(cls, path: str, feature_profile: str = "full") -> "SplatSubGraph":
        """Load a Gaussian-splat PLY and return a SplatSubGraph with metadata.

        Parameters
        ----------
        path : str
            Path to the PLY file.
        feature_profile : str
            ``'full'`` or ``'compact'``.

        Returns
        -------
        SplatSubGraph
        """
        sg, metadata = subgraph_from_ply_file(path, feature_profile=feature_profile)

        # Create SplatSubGraph and transfer state
        result = cls(sg.nnodes)
        result.nfeats = sg.nfeats
        result.nlabels = sg.nlabels

        for i in range(sg.nnodes):
            src_node = sg.get_node(i)
            dst_node = result.get_node(i)
            dst_node.feat = src_node.feat
            dst_node.position = src_node.position
            dst_node.label = src_node.label
            dst_node.truelabel = src_node.truelabel
            dst_node.pathval = src_node.pathval
            dst_node.pred = src_node.pred
            dst_node.status = src_node.status
            dst_node.relevant = src_node.relevant
            dst_node.root = src_node.root

        result.metadata = metadata
        return result

    def __repr__(self) -> str:
        """Pretty repr with metadata summary."""
        profile = self.metadata.get("feature_profile", "unknown")
        source = self.metadata.get("source", "(no source)")
        return (
            f"SplatSubGraph(nnodes={self.nnodes}, nfeats={self.nfeats}, "
            f"profile={profile!r}, source={source!r})"
        )

    def to_opf_model(self, path: str) -> None:
        """Export this SplatSubGraph to OPF binary training format.

        Parameters
        ----------
        path : str
            Path to write the OPF model file.
        """
        import opfpy
        opfpy.write_subgraph(path, self)

    def summary(self) -> str:
        """Return a text summary of the subgraph and its metadata."""
        lines = [
            f"SplatSubGraph Summary",
            f"  Nodes:        {self.nnodes}",
            f"  Features:     {self.nfeats}",
            f"  Labels:       {self.nlabels}",
            f"  Profile:      {self.metadata.get('feature_profile', 'unknown')}",
            f"  Source:       {self.metadata.get('source', '(none)')}",
            f"  Scene props:  {len(self.metadata.get('scene_properties', []))}",
        ]
        return "\n".join(lines)
   
    def to_ply_file(self, path: str, feature_profile: str = "full") -> None:
        """Export this SplatSubGraph to a PLY file with the given feature profile and label property."""
        write_subgraph_to_ply_file(self, path, metadata=getattr(self, "metadata", None), feature_profile=feature_profile)
