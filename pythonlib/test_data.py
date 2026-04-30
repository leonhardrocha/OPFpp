import unittest
from data import Node, Subgraph
import os

class TestDataStructures(unittest.TestCase):
    def test_node(self):
        node = Node(0, 1, [0.1, 0.2])
        self.assertEqual(node.index, 0)
        self.assertEqual(node.label, 1)
        self.assertEqual(node.features, [0.1, 0.2])

    def test_subgraph_io(self):
        nodes = [Node(i, i%2+1, [float(i), float(i+1)]) for i in range(3)]
        subgraph = Subgraph(nodes, nlabels=2, nfeats=2)
        test_file = 'test_opf.txt'
        subgraph.to_opf_file(test_file)
        loaded = Subgraph.from_opf_file(test_file)
        self.assertEqual(len(loaded.nodes), 3)
        self.assertEqual(loaded.nlabels, 2)
        self.assertEqual(loaded.nfeats, 2)
        for n1, n2 in zip(nodes, loaded.nodes):
            self.assertEqual(n1.index, n2.index)
            self.assertEqual(n1.label, n2.label)
            self.assertEqual(n1.features, n2.features)
        os.remove(test_file)

if __name__ == '__main__':
    unittest.main()
