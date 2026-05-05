"""Tests for the Cython wrapper layer (opfpy_cython) on top of opfpy."""
import unittest
import os
import sys

# Ensure the built extension is on the path when run from pythonlib/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'bin'))

# On Windows, the .pyd is built with MSYS2/UCRT64 GCC and requires runtime DLLs.
# Runtime folders are configured through VS Code settings using:
# UCRT64_RUNTIME_FOLDER and UCRT64_RUNTIME_LIB_FOLDER.
if sys.platform == "win32":
    _runtime_dirs = [
        os.environ.get("UCRT64_RUNTIME_FOLDER", ""),
        os.environ.get("UCRT64_RUNTIME_LIB_FOLDER", ""),
    ]
    for _runtime_dir in _runtime_dirs:
        if _runtime_dir and os.path.isdir(_runtime_dir):
            os.add_dll_directory(_runtime_dir)

import opfpy_cython as cy


class TestCythonNode(unittest.TestCase):
    def test_hello(self):
        self.assertEqual(cy.hello(), "Hello from OPF C++!")

    def test_node_properties(self):
        n = cy.Node()
        n.pathval = 1.5
        n.dens = 2.5
        n.radius = 3.5
        n.label = 4
        n.root = 5
        n.pred = 6
        n.truelabel = 7
        n.position = 8
        n.status = 1
        n.relevant = 1
        n.nplatadj = 2
        n.feat = [0.1, 0.2, 0.3]
        n.adj = [10, 20]

        self.assertAlmostEqual(n.pathval, 1.5, places=5)
        self.assertAlmostEqual(n.dens, 2.5, places=5)
        self.assertAlmostEqual(n.radius, 3.5, places=5)
        self.assertEqual(n.label, 4)
        self.assertEqual(n.root, 5)
        self.assertEqual(n.pred, 6)
        self.assertEqual(n.truelabel, 7)
        self.assertEqual(n.position, 8)
        self.assertEqual(n.status, 1)
        self.assertEqual(n.relevant, 1)
        self.assertEqual(n.nplatadj, 2)
        for got, exp in zip(n.feat, [0.1, 0.2, 0.3]):
            self.assertAlmostEqual(got, exp, places=5)
        self.assertEqual(n.adj, [10, 20])

    def test_node_adj_ops(self):
        n = cy.Node()
        n.adj = []
        n.add_to_adj(42)
        self.assertIn(42, n.adj)
        n.clear_adj()
        self.assertEqual(n.adj, [])

    def test_node_raw(self):
        """_raw property exposes the underlying opfpy.Node."""
        import opfpy
        n = cy.Node()
        self.assertIsInstance(n._raw, opfpy.Node)


class TestCythonSubgraph(unittest.TestCase):
    def test_subgraph_basic(self):
        sg = cy.Subgraph(3)
        sg.nfeats = 4
        sg.nlabels = 2
        sg.bestk = 1
        sg.df = 0.5
        sg.mindens = 0.1
        sg.maxdens = 0.9
        sg.K = 3

        self.assertEqual(sg.nnodes, 3)
        self.assertEqual(sg.nfeats, 4)
        self.assertEqual(sg.nlabels, 2)
        self.assertEqual(sg.bestk, 1)
        self.assertAlmostEqual(sg.df, 0.5, places=5)
        self.assertAlmostEqual(sg.mindens, 0.1, places=5)
        self.assertAlmostEqual(sg.maxdens, 0.9, places=5)
        self.assertEqual(sg.K, 3)

    def test_subgraph_get_node(self):
        sg = cy.Subgraph(2)
        sg.nfeats = 2
        n0 = sg.get_node(0)
        self.assertIsInstance(n0, cy.Node)
        n0.feat = [1.0, 2.0]
        n0.truelabel = 1
        got = sg.get_node(0)
        for a, b in zip(got.feat, [1.0, 2.0]):
            self.assertAlmostEqual(a, b, places=5)
        self.assertEqual(got.truelabel, 1)

    def test_subgraph_ordered_list(self):
        sg = cy.Subgraph(2)
        sg.clear_ordered_list_of_nodes()
        sg.add_ordered_node(0)
        sg.add_ordered_node(1)
        self.assertEqual(list(sg.ordered_list_of_nodes), [0, 1])
        sg.clear_ordered_list_of_nodes()
        self.assertEqual(list(sg.ordered_list_of_nodes), [])

    def test_subgraph_raw(self):
        """_raw property exposes the underlying opfpy.Subgraph."""
        import opfpy
        sg = cy.Subgraph()
        self.assertIsInstance(sg._raw, opfpy.Subgraph)

    def test_model_io_roundtrip(self):
        sg = cy.Subgraph(1)
        sg.nfeats = 2
        sg.nlabels = 1
        n = sg.get_node(0)
        n.feat = [0.25, 0.75]
        n.label = 1
        tmpfile = "_tmp_cy_model.bin"
        try:
            sg.write_model(tmpfile)
            sg2 = cy.Subgraph.read_model(tmpfile)
            self.assertIsInstance(sg2, cy.Subgraph)
            self.assertEqual(sg2.nfeats, 2)
            self.assertEqual(sg2.nnodes, 1)
            n2 = sg2.get_node(0)
            for a, b in zip(n2.feat, [0.25, 0.75]):
                self.assertAlmostEqual(a, b, places=5)
            self.assertEqual(n2.label, 1)
        finally:
            if os.path.exists(tmpfile):
                os.remove(tmpfile)

    def test_from_original_file(self):
        """from_original_file wraps the same opfpy function."""
        orig = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', 'data', 'boat.dat'))
        if not os.path.exists(orig):
            self.skipTest("Original file not available: " + orig)
        sg = cy.Subgraph.from_original_file(orig)
        self.assertIsInstance(sg, cy.Subgraph)
        self.assertGreater(sg.nnodes, 0)
        self.assertGreater(sg.nfeats, 0)


class TestCythonFreeFunctions(unittest.TestCase):
    def _make_opfpy_subgraph(self):
        import opfpy
        sg = opfpy.Subgraph(2)
        sg.nfeats = 2
        sg.nlabels = 1
        for i in range(2):
            n = sg.get_node(i)
            n.truelabel = 1
            n.position = i
            n.pathval = float(i) * 0.5
            n.feat = [float(i), float(i) + 0.1]
        return sg

    def test_write_read_roundtrip(self):
        import opfpy
        src = self._make_opfpy_subgraph()
        tmpfile = "_tmp_cy_rw.bin"
        try:
            cy.write_subgraph(tmpfile, src)
            sg2 = cy.read_subgraph(tmpfile)
            self.assertIsInstance(sg2, cy.Subgraph)
            self.assertEqual(sg2.nnodes, 2)
            self.assertEqual(sg2.nfeats, 2)
            n1 = sg2.get_node(1)
            self.assertAlmostEqual(n1.pathval, 0.5, places=5)
            for a, b in zip(n1.feat, [1.0, 1.1]):
                self.assertAlmostEqual(a, b, places=4)
        finally:
            if os.path.exists(tmpfile):
                os.remove(tmpfile)

    def test_write_cy_subgraph(self):
        """write_subgraph accepts a cy.Subgraph as well as opfpy.Subgraph."""
        sg = cy.Subgraph(1)
        sg.nfeats = 1
        sg.nlabels = 1
        n = sg.get_node(0)
        n.truelabel = 1
        n.position = 0
        n.pathval = 0.0
        n.feat = [3.14]
        tmpfile = "_tmp_cy_sg.bin"
        try:
            cy.write_subgraph(tmpfile, sg)
            sg2 = cy.read_subgraph(tmpfile)
            self.assertAlmostEqual(sg2.get_node(0).feat[0], 3.14, places=4)
        finally:
            if os.path.exists(tmpfile):
                os.remove(tmpfile)


if __name__ == "__main__":
    unittest.main()
