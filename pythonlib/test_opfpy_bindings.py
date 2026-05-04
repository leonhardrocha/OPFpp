import unittest
import os
import sys

# Ensure the built extension is on the path when run from pythonlib/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tools', '3rdparty', 'src'))

from opfpy import Node, Subgraph

class TestOPFBindings(unittest.TestCase):
    def test_node_properties(self):
        node = Node()
        node.pathval = 1.5
        node.dens = 2.5
        node.radius = 3.5
        node.label = 4
        node.root = 5
        node.pred = 6
        node.truelabel = 7
        node.position = 8
        node.status = 1
        node.relevant = 1
        node.nplatadj = 2
        node.feat = [0.1, 0.2, 0.3]
        node.adj = [1, 2, 3]
        self.assertEqual(node.pathval, 1.5)
        self.assertEqual(node.dens, 2.5)
        self.assertEqual(node.radius, 3.5)
        self.assertEqual(node.label, 4)
        self.assertEqual(node.root, 5)
        self.assertEqual(node.pred, 6)
        self.assertEqual(node.truelabel, 7)
        self.assertEqual(node.position, 8)
        self.assertEqual(node.status, 1)
        self.assertEqual(node.relevant, 1)
        self.assertEqual(node.nplatadj, 2)
        self.assertEqual(node.feat, [0.1, 0.2, 0.3])
        self.assertEqual(node.adj, [1, 2, 3])
        node.add_to_adj(4)
        self.assertIn(4, node.adj)
        node.clear_adj()
        self.assertEqual(node.adj, [])

    def test_subgraph_basic(self):
        sg = Subgraph(2)
        sg.nfeats = 3
        sg.nlabels = 2
        sg.bestk = 1
        sg.df = 0.5
        sg.mindens = 0.1
        sg.maxdens = 0.9
        sg.K = 2.0
        self.assertEqual(sg.nfeats, 3)
        self.assertEqual(sg.nlabels, 2)
        self.assertEqual(sg.bestk, 1)
        self.assertEqual(sg.df, 0.5)
        self.assertEqual(sg.mindens, 0.1)
        self.assertEqual(sg.maxdens, 0.9)
        self.assertEqual(sg.K, 2.0)
        self.assertEqual(sg.nnodes, 2)
        node0 = sg.get_node(0)
        node0.feat = [1.0, 2.0, 3.0]
        node0.label = 1
        sg.add_ordered_node(0)
        self.assertEqual(sg.ordered_list_of_nodes, [0])
        sg.clear_ordered_list_of_nodes()
        self.assertEqual(sg.ordered_list_of_nodes, [])

    def test_subgraph_model_io(self):
        sg = Subgraph(1)
        sg.nfeats = 2
        sg.nlabels = 1
        node = sg.get_node(0)
        node.feat = [0.5, 0.6]
        node.label = 1
        tmpfile = "_tmp_opf_model.bin"
        sg.write_model(tmpfile)
        sg2 = Subgraph.read_model(tmpfile)
        self.assertEqual(sg2.nfeats, 2)
        self.assertEqual(sg2.nlabels, 1)
        self.assertEqual(sg2.nnodes, 1)
        self.assertEqual(sg2.get_node(0).feat, [0.5, 0.6])
        self.assertEqual(sg2.get_node(0).label, 1)
        os.remove(tmpfile)


class TestFileIO(unittest.TestCase):
    """Tests for the OPF training-format free functions read_subgraph / write_subgraph."""

    def _make_subgraph(self):
        """Build a small Subgraph with 3 nodes and 2 features for roundtrip tests."""
        import opfpy
        sg = opfpy.Subgraph(3)
        sg.nfeats = 2
        sg.nlabels = 2
        for i in range(3):
            n = sg.get_node(i)
            n.truelabel = (i % 2) + 1
            n.position = i
            n.pathval = float(i) * 0.1
            n.feat = [float(i), float(i + 1)]
        return sg

    def test_write_read_roundtrip(self):
        """write_subgraph then read_subgraph must recover identical nodes."""
        import opfpy
        sg = self._make_subgraph()
        tmpfile = "_tmp_rw_subgraph.bin"
        try:
            opfpy.write_subgraph(tmpfile, sg)
            sg2 = opfpy.read_subgraph(tmpfile)
            self.assertEqual(sg2.nfeats, sg.nfeats)
            self.assertEqual(sg2.nlabels, sg.nlabels)
            self.assertEqual(sg2.nnodes, sg.nnodes)
            for i in range(sg.nnodes):
                self.assertEqual(sg2.get_node(i).truelabel, sg.get_node(i).truelabel)
                self.assertEqual(sg2.get_node(i).position, sg.get_node(i).position)
                self.assertAlmostEqual(sg2.get_node(i).pathval, sg.get_node(i).pathval, places=5)
                self.assertEqual(sg2.get_node(i).feat, sg.get_node(i).feat)
        finally:
            if os.path.exists(tmpfile):
                os.remove(tmpfile)

    def test_write_creates_file(self):
        """write_subgraph must create a non-empty file on disk."""
        import opfpy
        sg = self._make_subgraph()
        tmpfile = "_tmp_write_check.bin"
        try:
            opfpy.write_subgraph(tmpfile, sg)
            self.assertTrue(os.path.exists(tmpfile))
            self.assertGreater(os.path.getsize(tmpfile), 0)
        finally:
            if os.path.exists(tmpfile):
                os.remove(tmpfile)

    def test_read_nonexistent_raises(self):
        """read_subgraph must raise when the file does not exist."""
        import opfpy
        with self.assertRaises(Exception):
            opfpy.read_subgraph("_nonexistent_file_xyz.bin")

    def test_from_original_file_real_data(self):
        """from_original_file must load boat.dat with sensible structure."""
        dat = os.path.join(os.path.dirname(__file__), '..', 'data', 'boat.dat')
        dat = os.path.normpath(dat)
        if not os.path.exists(dat):
            self.skipTest(f"Real dataset not found: {dat}")
        sg = Subgraph.from_original_file(dat)
        self.assertGreater(sg.nnodes, 0, "Expected at least one node")
        self.assertGreater(sg.nfeats, 0, "Expected at least one feature")
        self.assertGreater(sg.nlabels, 0, "Expected at least one label")
        # Every node must have a feat vector of the right length
        for i in range(sg.nnodes):
            self.assertEqual(len(sg.get_node(i).feat), sg.nfeats)


if __name__ == "__main__":
    unittest.main()
