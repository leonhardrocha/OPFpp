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
        """Two tight clusters prepared by native kNN arc+PDF routines."""
        samples = [
            ([0.0, 0.0], 0),
            ([0.1, 0.1], 0),
            ([10.0, 10.0], 0),
            ([10.1, 9.9], 0),
        ]
        sg = _make_subgraph(samples, nfeats=2, nlabels=0)
        clf = opfpy.OPF()
        clf.create_arcs(sg, 2)
        clf.compute_pdf(sg)
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


class TestKernelSubgraphSplit(unittest.TestCase):
    def test_split_subgraph_into_kernels_and_cluster_smoke(self):
        """Smoke test for kernel splitting + unsupervised bestk workflow."""
        nnodes = 32
        nfeats = 12
        kernel_feature_sizes = [3, 3, 4, 2]

        sg = opfpy.Subgraph(nnodes)
        sg.nfeats = nfeats
        sg.nlabels = 0

        # Build two separable groups with deterministic features.
        for i in range(nnodes):
            base = 0.0 if i < (nnodes // 2) else 10.0
            feat = [base + (j * 0.01) + ((i % 4) * 0.001) for j in range(nfeats)]
            node = sg.get_node(i)
            node.feat = feat
            node.truelabel = 0
            node.position = i
            node.label = 0

        slices = []
        offset = 0
        for size in kernel_feature_sizes:
            slices.append((offset, size))
            offset += size

        kernels = opfpy.split_subgraph_into_kernels(sg, slices)
        self.assertEqual(len(kernels), len(kernel_feature_sizes))
        self.assertEqual([k.nfeats for k in kernels], kernel_feature_sizes)

        clf = opfpy.OPF()
        clf.bestk_min_cut(sg, 2, 5)
        clf.cluster(sg)

        # Ensure propagation produces non-zero labels from cluster roots.
        for i in range(sg.nnodes):
            if sg.get_node(i).root == i:
                sg.get_node(i).truelabel = int(sg.get_node(i).label) + 1
        opfpy.propagate_cluster_labels(sg)

        labels = [int(sg.get_node(i).label) for i in range(sg.nnodes)]
        self.assertEqual(len(labels), sg.nnodes)
        self.assertTrue(all(isinstance(lbl, int) for lbl in labels))
        self.assertGreater(sum(1 for lbl in labels if lbl > 0), 0)

    def test_named_kernel_slice_dict_density_and_cluster_per_kernel(self):
        """Split with named (offset,size) tuples and cluster each kernel separately."""
        nnodes = 24
        nfeats = 12

        sg = opfpy.Subgraph(nnodes)
        sg.nfeats = nfeats
        sg.nlabels = 0

        for i in range(nnodes):
            base = 0.0 if i < (nnodes // 2) else 8.0
            feat = [base + (j * 0.02) + ((i % 3) * 0.005) for j in range(nfeats)]
            node = sg.get_node(i)
            node.feat = feat
            node.truelabel = 0
            node.label = 0
            node.position = i

        kernel_slices = {
            "xyz": (0, 3),
            "f_dc": (3, 3),
            "f_rest": (6, 4),
            "opacity": (10, 2),
        }

        ordered_items = list(kernel_slices.items())
        all_slices = [kernel_slice for _, kernel_slice in ordered_items]
        kernels = opfpy.split_subgraph_into_kernels(sg, all_slices)

        self.assertEqual(len(kernels), len(ordered_items))

        for (kernel_name, kernel_slice), kernel in zip(ordered_items, kernels):
            # Convert each returned KernelSubGraph to plain Subgraph and run OPF independently.
            ksg = kernel.to_subgraph()
            self.assertEqual(ksg.nfeats, kernel_slice[1], msg=f"kernel={kernel_name}")

            clf = opfpy.OPF()
            clf.create_arcs(ksg, 2)
            clf.compute_pdf(ksg)
            clf.cluster(ksg)

            dens = [float(ksg.get_node(i).dens) for i in range(ksg.nnodes)]
            labels = [int(ksg.get_node(i).label) for i in range(ksg.nnodes)]

            self.assertEqual(len(dens), ksg.nnodes, msg=f"kernel={kernel_name}")
            self.assertTrue(all(d >= 0.0 for d in dens), msg=f"kernel={kernel_name}")
            self.assertEqual(len(labels), ksg.nnodes, msg=f"kernel={kernel_name}")
            self.assertGreater(ksg.nlabels, 0, msg=f"kernel={kernel_name}")


if __name__ == "__main__":
    unittest.main()
