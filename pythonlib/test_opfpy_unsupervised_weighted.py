import unittest
import os
import sys

# Ensure the built extension is on the path when run from pythonlib/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'bin'))

# Add MSYS2/UCRT64 runtime DLL directories on Windows
from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy


def _make_subgraph(samples, nfeats=4, nlabels=0):
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


def _configure_weighted_kernel(sg, sizes, weights):
    """Attach weighted-kernel metadata consumed by native PDF routines."""
    sg.kernel_feature_sizes = sizes
    sg.kernel_weights = weights


class TestWeightedClustering(unittest.TestCase):
    def _make_clusterable_weighted(self):
        """Two tight clusters with 2 kernel slices over 4D features."""
        samples = [
            ([0.0, 0.0, 0.0, 0.0], 0),
            ([0.1, 0.1, 0.0, 0.2], 0),
            ([10.0, 10.0, 5.0, 5.0], 0),
            ([9.9, 10.1, 5.2, 4.8], 0),
        ]
        sg = _make_subgraph(samples, nfeats=4, nlabels=0)
        _configure_weighted_kernel(sg, sizes=[2, 2], weights=[0.8, 0.2])

        clf = opfpy.OPF()
        clf.create_arcs(sg, 2)
        clf.compute_pdf(sg)
        return sg

    def test_cluster_assigns_labels_weighted(self):
        sg = self._make_clusterable_weighted()
        clf = opfpy.OPF()
        clf.cluster(sg)

        labels = [sg.get_node(i).label for i in range(sg.nnodes)]
        self.assertEqual(len(labels), sg.nnodes)
        self.assertTrue(all(isinstance(l, int) for l in labels))
        self.assertGreater(len(set(labels)), 0)

    def test_cluster_sets_nlabels_weighted(self):
        sg = self._make_clusterable_weighted()
        clf = opfpy.OPF()
        clf.cluster(sg)
        self.assertGreater(sg.nlabels, 0)


class TestWeightedBestKWorkflow(unittest.TestCase):
    def test_weighted_bestk_min_cut_workflow(self):
        """Run unsupervised best-k workflow with weighted kernel metadata."""
        data_path = os.path.join(os.path.dirname(__file__), "..", "data", "data1.dat")
        sg = opfpy.Subgraph.from_original_file(data_path)

        # data1.dat has 2 features, so define 2 one-dimensional slices.
        _configure_weighted_kernel(sg, sizes=[1, 1], weights=[0.6, 0.4])

        clf = opfpy.OPF()
        clf.bestk_min_cut(sg, 2, 10)
        opfpy.propagate_cluster_labels(sg)

        self.assertGreaterEqual(sg.bestk, 2)
        self.assertLessEqual(sg.bestk, 10)

        labels = {sg.get_node(i).label for i in range(sg.nnodes)}
        self.assertGreater(len(labels), 0)


class TestExample5WorkflowWeighted(unittest.TestCase):
    def test_example5_unsupervised_workflow_weighted(self):
        """Replicates example5 workflow but enables weighted kernels."""
        data_path = os.path.join(os.path.dirname(__file__), "..", "data", "data1.dat")

        # 1) Load dataset
        sg = opfpy.Subgraph.from_original_file(data_path)

        # 2) Configure weighted kernels for 2D features
        _configure_weighted_kernel(sg, sizes=[1, 1], weights=[0.6, 0.4])

        # 3) Unsupervised best-k min-cut setup
        clf = opfpy.OPF()
        clf.bestk_min_cut(sg, 2, 10)

        # 4) Propagate cluster labels
        opfpy.propagate_cluster_labels(sg)

        # 5) Ensure clustering produced labels
        labels = {sg.get_node(i).label for i in range(sg.nnodes)}
        self.assertGreater(len(labels), 0)

        # 6) Split and run k-NN classify
        train_sg, test_sg = opfpy.split_subgraph(sg, 0.5)
        clf.knn_classify(train_sg, test_sg)

        # 7) Accuracy is a valid probability in [0, 1]
        acc = clf.accuracy(test_sg)
        self.assertGreaterEqual(acc, 0.0)
        self.assertLessEqual(acc, 1.0)


if __name__ == "__main__":
    unittest.main()
