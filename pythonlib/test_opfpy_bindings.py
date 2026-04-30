import unittest
import os
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

if __name__ == "__main__":
    unittest.main()
