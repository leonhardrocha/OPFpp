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


class TestKernelBestKModes(unittest.TestCase):
    def _make_unsup_subgraph(self, nnodes=40, nfeats=6):
        sg = opfpy.Subgraph(nnodes)
        sg.nfeats = nfeats
        sg.nlabels = 0
        for i in range(nnodes):
            base = 0.0 if i < (nnodes // 2) else 6.0
            feat = [base + (j * 0.015) + ((i % 5) * 0.003) for j in range(nfeats)]
            node = sg.get_node(i)
            node.feat = feat
            node.truelabel = 0
            node.label = 0
            node.position = i
        return sg

    def _make_kernel_sensitive_subgraph(self):
        samples = []
        for i in range(12):
            c = 0.0 if i < 6 else 1.0
            feat = [
                c * 3.0 + (i % 3) * 0.02,
                c * 3.0 + (i % 2) * 0.03,
                c * 1.0 + (i % 4) * 0.09,
                c * 1.0 + (i % 5) * 0.07,
                c * 0.3 + (i % 6) * 0.12,
                c * 0.3 + (i % 7) * 0.11,
            ]
            samples.append((feat, 0))
        return _make_subgraph(samples, nfeats=6, nlabels=0)

    def test_density_estimation_mode_changes_density_values(self):
        density_mode = getattr(opfpy, "DensityEstimationMode", None)
        if density_mode is None:
            self.skipTest("DensityEstimationMode enum not available in this build")

        sg_gauss = self._make_unsup_subgraph(nnodes=20, nfeats=4)
        sg_inv = self._make_unsup_subgraph(nnodes=20, nfeats=4)

        clf_gauss = opfpy.OPF()
        clf_inv = opfpy.OPF()

        clf_gauss.set_density_estimation_mode(density_mode.GAUSSIAN)
        clf_inv.set_density_estimation_mode(density_mode.INVERSE_DISTANCE)

        clf_gauss.create_arcs(sg_gauss, 3)
        clf_inv.create_arcs(sg_inv, 3)
        clf_gauss.compute_pdf(sg_gauss)
        clf_inv.compute_pdf(sg_inv)

        dens_gauss = [float(sg_gauss.get_node(i).dens) for i in range(sg_gauss.nnodes)]
        dens_inv = [float(sg_inv.get_node(i).dens) for i in range(sg_inv.nnodes)]

        self.assertEqual(len(dens_gauss), len(dens_inv))
        self.assertTrue(any(abs(a - b) > 1e-6 for a, b in zip(dens_gauss, dens_inv)))

    def test_preset_adjacency_mode_keeps_existing_edges(self):
        adjacency_mode = getattr(opfpy, "AdjacencyMode", None)
        if adjacency_mode is None:
            self.skipTest("AdjacencyMode enum not available in this build")

        sg = self._make_unsup_subgraph(nnodes=8, nfeats=3)
        expected_adj = {}
        for i in range(sg.nnodes):
            node = sg.get_node(i)
            node.clear_adj()
            a = (i + 1) % sg.nnodes
            b = (i + 3) % sg.nnodes
            node.add_to_adj(a)
            node.add_to_adj(b)
            expected_adj[i] = [a, b]

        clf = opfpy.OPF()
        clf.set_adjacency_mode(adjacency_mode.PRESET)
        clf.create_arcs(sg, 2)

        for i in range(sg.nnodes):
            self.assertEqual(list(sg.get_node(i).adj), expected_adj[i])
        self.assertEqual(int(sg.bestk), 2)
        self.assertGreater(float(sg.df), 0.0)

    def test_bestk_min_cut_per_kernel_returns_results(self):
        if getattr(opfpy, "split_subgraph_into_kernels", None) is None:
            self.skipTest("split_subgraph_into_kernels not available")
        if getattr(opfpy, "KernelBestKResult", None) is None:
            self.skipTest("KernelBestKResult not available in this build")

        sg = self._make_kernel_sensitive_subgraph()
        kernels = opfpy.split_subgraph_into_kernels(sg, [(0, 2), (2, 2), (4, 2)])

        clf = opfpy.OPF()
        results = clf.bestk_min_cut_per_kernel(kernels, 2, 4)

        self.assertEqual(len(results), 3)
        bestks = []
        dfs = []
        for idx, res in enumerate(results):
            self.assertEqual(int(res.kernel_id), idx)
            self.assertGreaterEqual(int(res.bestk), 2)
            self.assertLessEqual(int(res.bestk), 4)
            self.assertGreater(float(res.df_at_bestk), 0.0)
            bestks.append(int(res.bestk))
            dfs.append(float(res.df_at_bestk))

        self.assertTrue(any(abs(a - b) > 1e-6 for a, b in zip(dfs, dfs[1:])))
        self.assertEqual(len(bestks), 3)

    def test_kernel_adjacency_changes_with_k_and_can_differ(self):
        if getattr(opfpy, "split_subgraph_into_kernels", None) is None:
            self.skipTest("split_subgraph_into_kernels not available")

        sg = self._make_kernel_sensitive_subgraph()
        kernels = opfpy.split_subgraph_into_kernels(sg, [(0, 2), (2, 2), (4, 2)])

        clf = opfpy.OPF()
        low_k = 2
        high_k = 10

        kernel_signatures = []
        for kernel in kernels:
            sg_low = kernel.to_subgraph()
            sg_high = kernel.to_subgraph()

            clf.create_arcs(sg_low, low_k)
            clf.create_arcs(sg_high, high_k)

            low_sizes = [len(sg_low.get_node(i).adj) for i in range(sg_low.nnodes)]
            high_sizes = [len(sg_high.get_node(i).adj) for i in range(sg_high.nnodes)]

            self.assertTrue(all(s == low_k for s in low_sizes))
            self.assertTrue(all(s == high_k for s in high_sizes))
            self.assertGreater(sum(high_sizes), sum(low_sizes))

            # Capture adjacency signature under the same k to verify kernels may differ.
            kernel_signatures.append(tuple(tuple(sg_low.get_node(i).adj) for i in range(sg_low.nnodes)))

        self.assertGreater(len(set(kernel_signatures)), 1)


if __name__ == "__main__":
    unittest.main()
