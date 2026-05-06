"""
opfppy.distance — DistanceMetric enum and resolver factory.

Usage
-----
>>> from opfppy.distance import DistanceMetric, resolve
>>> resolve("manhattan")       # → 3
>>> resolve(DistanceMetric.MANHATTAN)  # → 3
>>> resolve(3)                 # → 3

Extending
---------
Call ``register(name, id)`` to add a custom metric that maps to an integer
id already implemented at the C++ layer::

    from opfppy.distance import register
    register("my_metric", 8)   # after rebuilding opfpy with id 8 support

"""

from __future__ import annotations

from enum import IntEnum
from typing import Union


class DistanceMetric(IntEnum):
    """Named identifiers for the built-in OPF distance functions."""
    EUCLIDEAN          = 1
    CHI_SQUARED        = 2
    MANHATTAN          = 3
    CANBERRA           = 4
    SQUARED_CHORD      = 5
    SQUARED_CHI_SQUARED = 6
    BRAY_CURTIS        = 7


# ---------------------------------------------------------------------------
# Registry — maps normalised string → int id
# Built from the enum; additional entries can be added via register().
# ---------------------------------------------------------------------------

def _normalise(name: str) -> str:
    """Lower-case and strip hyphens/underscores/spaces for loose matching."""
    return name.lower().replace("-", "").replace("_", "").replace(" ", "")


_REGISTRY: dict[str, int] = {
    _normalise(m.name): int(m) for m in DistanceMetric
}

# Common aliases
_ALIASES: dict[str, str] = {
    "eucl":               "euclidean",
    "euclid":             "euclidean",
    "chi2":               "chisquared",
    "chisq":              "chisquared",
    "squaredchord":       "squaredchord",
    "squaredchisquared":  "squaredchisquared",
    "braycurtis":         "braycurtis",
    "bray":               "braycurtis",
    "l1":                 "manhattan",
    "l2":                 "euclidean",
    "cityblock":          "manhattan",
}
for _alias, _target in _ALIASES.items():
    if _normalise(_target) in _REGISTRY:
        _REGISTRY[_normalise(_alias)] = _REGISTRY[_normalise(_target)]


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

DistanceSpec = Union[int, str, DistanceMetric]


def resolve(distance: DistanceSpec) -> int:
    """Resolve *distance* to the integer id expected by opfpy.

    Parameters
    ----------
    distance : int | str | DistanceMetric
        * ``int`` — passed through unchanged (validated against registry).
        * ``str`` — matched case-insensitively, ignoring hyphens/underscores.
        * ``DistanceMetric`` — enum member, returned as its integer value.

    Returns
    -------
    int  The distance id (1–7 for built-in metrics).

    Raises
    ------
    ValueError  If the input cannot be resolved.
    TypeError   If the input type is unsupported.
    """
    if isinstance(distance, DistanceMetric):
        return int(distance)

    if isinstance(distance, int):
        if distance not in {int(m) for m in DistanceMetric} and distance not in _REGISTRY.values():
            raise ValueError(
                f"Unknown distance id {distance!r}. "
                f"Built-in ids: {sorted({int(m) for m in DistanceMetric})}."
            )
        return distance

    if isinstance(distance, str):
        key = _normalise(distance)
        if key not in _REGISTRY:
            known = sorted({m.name.lower() for m in DistanceMetric})
            raise ValueError(
                f"Unknown distance name {distance!r}. "
                f"Known names: {known}."
            )
        return _REGISTRY[key]

    raise TypeError(
        f"distance must be int, str, or DistanceMetric, got {type(distance).__name__!r}."
    )


def register(name: str, distance_id: int) -> None:
    """Register a custom distance name→id mapping.

    This is the extension point for future metrics added to the C++ layer.

    Parameters
    ----------
    name        : str   Arbitrary name (stored normalised).
    distance_id : int   Integer id that the opfpy C++ layer will accept.
    """
    _REGISTRY[_normalise(name)] = distance_id
