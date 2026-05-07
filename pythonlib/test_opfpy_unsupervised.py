import unittest
import os
import sys

# Ensure the built extension is on the path when run from pythonlib/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'bin'))

# Add MSYS2/UCRT64 runtime DLL directories on Windows
from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy


def _make_subgraph(samples, nfeats=2, nlabels=0):
    """Build a Subgraph from a list of (feat, truelabel) tuples."""
    sg = opfpy.Subgraph(len(samples))
    sg.nfeats = nfeats
    sg.nlabels = nlabels
    for i, (feat, truelabel) in enumerate(samples):
        node = sg.get_node(i)
        node.feat = feat
        node.truelabel = truelabel
        node.label = 0
        node.position = i
        node.pathval = 0.0
        node.pred = -1
        node.status = 0
        node.relevant = 0
        node.root = i
    return sg


class TestClustering(unittest.TestCase):
    def _make_clusterable(self):
        """Two tight clusters with dens + full adjacency (as opf_cluster does)."""
        samples = [
            ([0.0, 0.0], 0),
            ([0.1, 0.1], 0),
            ([10.0, 10.0], 0),
            ([10.1, 9.9], 0),
        ]
        sg = _make_subgraph(samples, nfeats=2, nlabels=0)
        # Assign density inversely proportional to inter-node distance
        # (closer nodes = higher density), and full adjacency
        import math
        n = sg.nnodes
        for i in range(n):
            fi = sg.get_node(i).feat
            min_d = float('inf')
            for j in range(n):
                if i != j:
                    fj = sg.get_node(j).feat
                    d = math.sqrt(sum((a - b) ** 2 for a, b in zip(fi, fj)))
                    min_d = min(min_d, d)
            sg.get_node(i).dens = 1.0 / (1.0 + min_d)
            sg.get_node(i).pathval = sg.get_node(i).dens
            for j in range(n):
                if i != j:
                    sg.get_node(i).add_to_adj(j)
        return sg

    def test_cluster_assigns_labels(self):
        sg = self._make_clusterable()
        clf = opfpy.OPF()
        clf.cluster(sg)

        labels = [sg.get_node(i).label for i in range(sg.nnodes)]
        self.assertEqual(len(labels), sg.nnodes)
        self.assertTrue(all(isinstance(l, int) for l in labels))
        # Must have produced at least one cluster
        self.assertGreater(len(set(labels)), 0)

    def test_cluster_sets_nlabels(self):
        sg = self._make_clusterable()
        clf = opfpy.OPF()
        clf.cluster(sg)
        # nlabels is updated by clustering to the number of found clusters
        self.assertGreater(sg.nlabels, 0)

    def test_propagate_cluster_labels(self):
        sg = self._make_clusterable()
        clf = opfpy.OPF()
        clf.cluster(sg)
        # Set root nodes' truelabel so propagation produces non-zero labels
        for i in range(sg.nnodes):
            if sg.get_node(i).root == i:
                sg.get_node(i).truelabel = sg.get_node(i).label + 1
        opfpy.propagate_cluster_labels(sg)
        labels = [sg.get_node(i).label for i in range(sg.nnodes)]
        self.assertTrue(all(l >= 0 for l in labels))


class TestKnnClassify(unittest.TestCase):
    def test_knn_classify(self):
        """k-NN classification using manually set radius on training nodes."""
        train = _make_subgraph([
            ([0.0, 0.0], 1),
            ([0.2, 0.1], 1),
            ([10.0, 10.0], 2),
            ([10.2, 9.9], 2),
        ], nfeats=2, nlabels=2)
        test = _make_subgraph([
            ([0.1, 0.0], 1),
            ([10.1, 10.0], 2),
        ], nfeats=2, nlabels=2)

        # Supervised training to populate ordered_list_of_nodes and pathval
        clf = opfpy.OPF()
        clf.train(train)

        # Set a radius on each training node that covers nearby test points
        for i in range(train.nnodes):
            train.get_node(i).radius = 1.0

        clf.knn_classify(train, test)

        labels = [test.get_node(i).label for i in range(test.nnodes)]
        self.assertEqual(labels[0], 1)
        self.assertEqual(labels[1], 2)


class TestSemiSupervised(unittest.TestCase):
    def test_semi_supervised_no_eval(self):
        """semi_supervised without eval returns a merged trained subgraph."""
        labeled = _make_subgraph([
            ([0.0, 0.0], 1),
            ([10.0, 10.0], 2),
        ], nfeats=2, nlabels=2)
        unlabeled = _make_subgraph([
            ([0.5, 0.5], 0),
            ([9.5, 9.5], 0),
        ], nfeats=2, nlabels=2)

        clf = opfpy.OPF()
        merged = clf.semi_supervised(labeled, unlabeled)

        self.assertIsInstance(merged, opfpy.Subgraph)
        self.assertEqual(merged.nnodes, labeled.nnodes + unlabeled.nnodes)

    def test_semi_supervised_with_eval(self):
        """semi_supervised with eval subgraph completes without error."""
        labeled = _make_subgraph([
            ([0.0, 0.0], 1),
            ([10.0, 10.0], 2),
        ], nfeats=2, nlabels=2)
        unlabeled = _make_subgraph([
            ([0.3, 0.2], 0),
            ([9.8, 9.9], 0),
        ], nfeats=2, nlabels=2)
        eval_sg = _make_subgraph([
            ([0.1, 0.0], 1),
            ([10.1, 10.0], 2),
        ], nfeats=2, nlabels=2)

        clf = opfpy.OPF()
        merged = clf.semi_supervised(labeled, unlabeled, eval_sg)

        self.assertIsInstance(merged, opfpy.Subgraph)
        self.assertGreater(merged.nnodes, 0)


if __name__ == "__main__":
    unittest.main()
