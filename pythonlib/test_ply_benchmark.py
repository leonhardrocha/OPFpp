"""
test_ply_benchmark.py — performance benchmark for PLY loading (full vs compact).

Measures load time, feature vector size, and memory footprint for both profiles.
"""

import os
import sys
import tempfile
import time
import unittest

import numpy as np
from plyfile import PlyData, PlyElement

from opfppy.ply_adapter import SplatSubGraph


def _vertex_dtype():
    """Full Gaussian splat schema."""
    names = [
        "x", "y", "z",
        "nx", "ny", "nz",
        "f_dc_0", "f_dc_1", "f_dc_2",
        *[f"f_rest_{i}" for i in range(45)],
        "opacity",
        "scale_0", "scale_1", "scale_2",
        "rot_0", "rot_1", "rot_2", "rot_3",
    ]
    return np.dtype([(name, "f4") for name in names])


def _build_test_ply(n_vertices=1000):
    """Create a temporary PLY file with n_vertices."""
    vertices = np.zeros(n_vertices, dtype=_vertex_dtype())
    for i in range(n_vertices):
        vertices["x"][i] = float(i) * 0.1
        vertices["y"][i] = float(i) * 0.2
        vertices["z"][i] = float(i) * 0.3
        for j in range(45):
            vertices[f"f_rest_{j}"][i] = float(j) * 0.01
        vertices["opacity"][i] = 0.5 + (i % 10) * 0.01
        vertices["scale_0"][i] = 1.0
        vertices["scale_1"][i] = 1.0
        vertices["scale_2"][i] = 1.0
        vertices["rot_0"][i] = 1.0

    elem = PlyElement.describe(vertices, "vertex")
    ply = PlyData([elem], text=False)

    fd, path = tempfile.mkstemp(suffix=".ply")
    os.close(fd)
    ply.write(path)
    return path


class TestSplatSubGraphBasic(unittest.TestCase):
    """Basic functionality tests for SplatSubGraph."""

    def test_from_ply_preserves_nnodes(self):
        path = _build_test_ply(100)
        try:
            sg_full = SplatSubGraph.from_ply_file(path, feature_profile="full")
            self.assertEqual(sg_full.nnodes, 100)
            self.assertIn("source", sg_full.metadata)
            self.assertEqual(sg_full.metadata["source"], path)
        finally:
            os.unlink(path)

    def test_repr_includes_metadata(self):
        path = _build_test_ply(50)
        try:
            sg = SplatSubGraph.from_ply_file(path, feature_profile="compact")
            repr_str = repr(sg)
            self.assertIn("SplatSubGraph", repr_str)
            self.assertIn("compact", repr_str)
        finally:
            os.unlink(path)

    def test_metadata_contains_sh_codes(self):
        path = _build_test_ply(10)
        try:
            sg = SplatSubGraph.from_ply_file(path, feature_profile="full")
            self.assertIn("sh_codes", sg.metadata)
            self.assertIn("f_dc_0", sg.metadata["sh_codes"])
            self.assertEqual(sg.metadata["sh_codes"]["f_dc_0"], 16)
        finally:
            os.unlink(path)


class TestPlyBenchmark(unittest.TestCase):
    """Benchmark: full vs compact profile loading and memory."""

    def test_benchmark_load_time_and_memory(self):
        """Compare load time and feature vector sizes for full vs compact."""
        n_vertices = 1000
        path = _build_test_ply(n_vertices)

        try:
            # Full profile benchmark
            t0_full = time.perf_counter()
            sg_full = SplatSubGraph.from_ply_file(path, feature_profile="full")
            t1_full = time.perf_counter()
            time_full = t1_full - t0_full

            # Compact profile benchmark
            t0_compact = time.perf_counter()
            sg_compact = SplatSubGraph.from_ply_file(path, feature_profile="compact")
            t1_compact = time.perf_counter()
            time_compact = t1_compact - t0_compact

            # Assertions and reporting
            self.assertEqual(sg_full.nnodes, n_vertices)
            self.assertEqual(sg_compact.nnodes, n_vertices)

            nfeats_full = sg_full.nfeats
            nfeats_compact = sg_compact.nfeats

            print(f"\nBenchmark ({n_vertices} vertices):")
            print(f"  Full profile:")
            print(f"    Load time:  {time_full*1000:.2f} ms")
            print(f"    Features:   {nfeats_full}")
            print(f"  Compact profile:")
            print(f"    Load time:  {time_compact*1000:.2f} ms")
            print(f"    Features:   {nfeats_compact}")
            print(f"  Memory savings: {(1 - nfeats_compact/nfeats_full)*100:.1f}%")
            print(f"  Speed ratio (compact/full): {time_compact/time_full:.2f}x")

            # Compact should have fewer features
            self.assertLess(nfeats_compact, nfeats_full)

            # Both should load in reasonable time (< 1s for 1000 vertices)
            self.assertLess(time_full, 1.0)
            self.assertLess(time_compact, 1.0)

        finally:
            os.unlink(path)

    def test_feature_access_performance(self):
        """Benchmark feature vector access patterns."""
        n_vertices = 500
        path = _build_test_ply(n_vertices)

        try:
            sg_full = SplatSubGraph.from_ply_file(path, feature_profile="full")
            sg_compact = SplatSubGraph.from_ply_file(path, feature_profile="compact")

            # Time full profile access
            t0 = time.perf_counter()
            for i in range(n_vertices):
                node = sg_full.get_node(i)
                _ = node.feat
            t1 = time.perf_counter()
            time_full = t1 - t0

            # Time compact profile access
            t0 = time.perf_counter()
            for i in range(n_vertices):
                node = sg_compact.get_node(i)
                _ = node.feat
            t1 = time.perf_counter()
            time_compact = t1 - t0

            print(f"\nFeature Access Benchmark ({n_vertices} nodes):")
            print(f"  Full:    {time_full*1000:.2f} ms")
            print(f"  Compact: {time_compact*1000:.2f} ms")
            print(f"  Speed-up: {time_full/time_compact:.2f}x")

            # Compact should be faster or comparable
            self.assertLess(time_compact, time_full * 1.5)

        finally:
            os.unlink(path)


if __name__ == "__main__":
    unittest.main()
