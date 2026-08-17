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

from array import array
from enum import IntEnum
from typing import Any, Callable, Union


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


_DISTANCE_FN_BY_ID: dict[int, str] = {
    int(DistanceMetric.EUCLIDEAN): "eucl_dist",
    int(DistanceMetric.CHI_SQUARED): "chi_squared_dist",
    int(DistanceMetric.MANHATTAN): "manhattan_dist",
    int(DistanceMetric.CANBERRA): "canberra_dist",
    int(DistanceMetric.SQUARED_CHORD): "squared_chord_dist",
    int(DistanceMetric.SQUARED_CHI_SQUARED): "squared_chi_squared_dist",
    int(DistanceMetric.BRAY_CURTIS): "bray_curtis_dist",
}


def _detect_precision(features_a: Any, features_b: Any) -> str:
    """Infer target precision from runtime feature containers.

    Returns "double" when either input explicitly carries double precision
    metadata; otherwise returns "float" for backward compatibility.
    """

    def _from_obj(obj: Any) -> str | None:
        dtype = getattr(obj, "dtype", None)
        if dtype is not None:
            text = str(dtype).lower()
            if "float64" in text or "double" in text:
                return "double"
            if "float32" in text or "single" in text:
                return "float"

        if isinstance(obj, array):
            if obj.typecode == "d":
                return "double"
            if obj.typecode == "f":
                return "float"

        return None

    pa = _from_obj(features_a)
    pb = _from_obj(features_b)

    if pa == "double" or pb == "double":
        return "double"
    return "float"


def _resolve_distance_callable(distance: DistanceSpec, features_a: Any, features_b: Any) -> Callable[[Any, Any], float]:
    import opfpy

    distance_id = resolve(distance)
    base_name = _DISTANCE_FN_BY_ID.get(distance_id)
    if base_name is None:
        raise ValueError(f"No callable registered for distance id {distance_id!r}.")

    if _detect_precision(features_a, features_b) == "double":
        return getattr(opfpy, f"{base_name}_double")
    return getattr(opfpy, base_name)


def distance(features_a: Any, features_b: Any, metric: DistanceSpec = DistanceMetric.EUCLIDEAN) -> float:
    """Compute distance with runtime float/double dispatch.

    The template specialization is chosen dynamically:
    - float path: default and backward-compatible behavior
    - double path: selected when inputs carry explicit double metadata
      (e.g. numpy float64 dtype or array('d'))
    """
    fn = _resolve_distance_callable(metric, features_a, features_b)
    return float(fn(features_a, features_b))


def eucl_dist(features_a: Any, features_b: Any) -> float:
    return distance(features_a, features_b, DistanceMetric.EUCLIDEAN)


def chi_squared_dist(features_a: Any, features_b: Any) -> float:
    return distance(features_a, features_b, DistanceMetric.CHI_SQUARED)


def manhattan_dist(features_a: Any, features_b: Any) -> float:
    return distance(features_a, features_b, DistanceMetric.MANHATTAN)


def canberra_dist(features_a: Any, features_b: Any) -> float:
    return distance(features_a, features_b, DistanceMetric.CANBERRA)


def squared_chord_dist(features_a: Any, features_b: Any) -> float:
    return distance(features_a, features_b, DistanceMetric.SQUARED_CHORD)


def squared_chi_squared_dist(features_a: Any, features_b: Any) -> float:
    return distance(features_a, features_b, DistanceMetric.SQUARED_CHI_SQUARED)


def bray_curtis_dist(features_a: Any, features_b: Any) -> float:
    return distance(features_a, features_b, DistanceMetric.BRAY_CURTIS)


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
