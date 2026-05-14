import io
import os
import tempfile
import unittest

import numpy as np
from plyfile import PlyData, PlyElement

from opfppy.ply_adapter import decode_sh_params, encode_sh_params, subgraph_from_ply_file


def _vertex_dtype():
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


def _build_vertices(n=2):
    vertices = np.zeros(n, dtype=_vertex_dtype())
    for i in range(n):
        vertices["x"][i] = float(i)
        vertices["y"][i] = float(i + 1)
        vertices["z"][i] = float(i + 2)
        vertices["opacity"][i] = 0.5
        vertices["scale_0"][i] = 1.0
        vertices["scale_1"][i] = 1.0
        vertices["scale_2"][i] = 1.0
        vertices["rot_0"][i] = 1.0
    return vertices


class TestSHEncoding(unittest.TestCase):
    def test_examples_roundtrip(self):
        cases = [
            (0, 0, 16),
            (1, -1, 47),
            (2, 2, 82),
            (3, 3, 115),
        ]
        for l, m, expected in cases:
            code = encode_sh_params(l, m)
            self.assertEqual(code, expected)
            self.assertEqual(decode_sh_params(code), (l, m))

    def test_invalid_ranges(self):
        with self.assertRaises(ValueError):
            encode_sh_params(8, 0)
        with self.assertRaises(ValueError):
            encode_sh_params(1, 16)
        with self.assertRaises(ValueError):
            decode_sh_params(999)


class TestPlyAdapter(unittest.TestCase):

    def test_from_ply_file_full_and_label(self):
        from opfppy.ply_adapter import write_subgraph_to_ply_file, node_scalar_members
        vertices = _build_vertices(3)
        elem = PlyElement.describe(vertices, "vertex")

        fd, path = tempfile.mkstemp(suffix=".ply")
        os.close(fd)
        try:
            PlyData([elem], text=False).write(path)
            sg, meta = subgraph_from_ply_file(path, feature_profile="full")

            # Add label values
            for i in range(sg.nnodes):
                sg.get_node(i).label = 42 + i

            # Write with label in profile
            fd2, outpath = tempfile.mkstemp(suffix=".ply")
            os.close(fd2)
            try:
                write_subgraph_to_ply_file(sg, outpath, metadata=meta, feature_profile="full+label")
                # Read back and check header
                with open(outpath, "rb") as f:
                    header = b""
                    while True:
                        line = f.readline()
                        header += line
                        if line.strip() == b'end_header':
                            break
                    self.assertIn(b'label', header)
                # Check binary data for label values
                with open(outpath, 'rb') as f:
                    ply = PlyData.read(io.BytesIO(f.read()))
                arr = ply["vertex"].data
                self.assertIn("label", arr.dtype.names)
                for i in range(sg.nnodes):
                    self.assertEqual(arr["label"][i], 42 + i)
            finally:
                os.unlink(outpath)
        finally:
            os.unlink(path)

    def test_from_ply_file_compact_and_label(self):
        from opfppy.ply_adapter import write_subgraph_to_ply_file
        vertices = _build_vertices(2)
        elem = PlyElement.describe(vertices, "vertex")

        fd, path = tempfile.mkstemp(suffix=".ply")
        os.close(fd)
        try:
            PlyData([elem], text=False).write(path)
            sg, meta = subgraph_from_ply_file(path, feature_profile="compact")
            for i in range(sg.nnodes):
                sg.get_node(i).label = 100 + i

            fd2, outpath = tempfile.mkstemp(suffix=".ply")
            os.close(fd2)
            try:
                write_subgraph_to_ply_file(sg, outpath, metadata=meta, feature_profile="compact+label")
                with open(outpath, "rb") as f:
                    header = b""
                    while True:
                        line = f.readline()
                        header += line
                        if line.strip() == b'end_header':
                            break
                    self.assertIn(b'label', header)
                with open(outpath, 'rb') as f:
                    ply = PlyData.read(io.BytesIO(f.read()))
                arr = ply["vertex"].data
                self.assertIn("label", arr.dtype.names)
                for i in range(sg.nnodes):
                    self.assertEqual(arr["label"][i], 100 + i)
            finally:
                os.unlink(outpath)
        finally:
            os.unlink(path)


if __name__ == "__main__":
    unittest.main()
