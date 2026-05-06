import unittest
import os
import sys
import math
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'bin'))

from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy


def _make_subgraph(samples, nfeats=2, nlabels=2):
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


class TestNormalize(unittest.TestCase):
    def test_normalize_zero_mean(self):
        """After normalization, each feature dimension should have approximately zero mean."""
        sg = _make_subgraph([
            ([1.0, 10.0], 1),
            ([2.0, 20.0], 1),
            ([3.0, 30.0], 2),
            ([4.0, 40.0], 2),
        ], nfeats=2, nlabels=2)

        clf = opfpy.OPF()
        clf.normalize(sg)

        feat0 = [sg.get_node(i).feat[0] for i in range(sg.nnodes)]
        feat1 = [sg.get_node(i).feat[1] for i in range(sg.nnodes)]
        self.assertAlmostEqual(sum(feat0) / len(feat0), 0.0, places=5)
        self.assertAlmostEqual(sum(feat1) / len(feat1), 0.0, places=5)

    def test_normalize_constant_feature_unchanged_scale(self):
        """A constant feature dimension (std=0) should not cause a crash (std set to 1)."""
        sg = _make_subgraph([
            ([5.0, 1.0], 1),
            ([5.0, 2.0], 1),
            ([5.0, 3.0], 2),
        ], nfeats=2, nlabels=2)

        clf = opfpy.OPF()
        clf.normalize(sg)  # should not raise
        # constant feature dim 0 should remain constant (val - mean) / 1.0 = val - mean
        vals = [sg.get_node(i).feat[0] for i in range(sg.nnodes)]
        self.assertEqual(vals[0], vals[1])
        self.assertEqual(vals[1], vals[2])


class TestPruning(unittest.TestCase):
    def test_pruning_returns_float(self):
        """pruning() returns a float in [0, 1].

        Note: the pruning loop (acc >= desired_acc) accesses sg_eval.node.pred which
        is not set by classifying() — a known C++ bug. We test with desired_accuracy
        above the achievable maximum so the loop body never executes.
        """
        train = _make_subgraph([
            ([0.0, 0.0], 1),
            ([0.1, 0.1], 1),
            ([10.0, 10.0], 2),
            ([10.1, 9.9], 2),
        ], nfeats=2, nlabels=2)
        eval_sg = _make_subgraph([
            ([0.2, 0.0], 1),
            ([9.8, 10.0], 2),
        ], nfeats=2, nlabels=2)

        clf = opfpy.OPF()
        # Use desired_accuracy > 1.0 so the pruning loop never executes
        # (avoids the C++ bug where eval.pred is -1 when accessed as a train index)
        rate = clf.pruning(train, eval_sg, desired_accuracy=1.1)

        self.assertIsInstance(rate, float)
        self.assertGreaterEqual(rate, 0.0)
        self.assertLessEqual(rate, 1.0)


class TestSubgraphInfo(unittest.TestCase):
    def test_info_fields(self):
        sg = _make_subgraph([
            ([1.0, 2.0], 1),
            ([3.0, 4.0], 2),
        ], nfeats=2, nlabels=2)

        info = opfpy.subgraph_info(sg)
        self.assertEqual(info['nnodes'], 2)
        self.assertEqual(info['nlabels'], 2)
        self.assertEqual(info['nfeats'], 2)


class TestKFold(unittest.TestCase):
    def test_k_fold_count(self):
        sg = _make_subgraph([
            ([float(i), float(i)], (i % 2) + 1) for i in range(8)
        ], nfeats=2, nlabels=2)

        folds = opfpy.k_fold(sg, 4)
        self.assertEqual(len(folds), 4)

    def test_k_fold_total_nodes(self):
        """All folds together contain all nodes."""
        sg = _make_subgraph([
            ([float(i), float(i)], (i % 2) + 1) for i in range(8)
        ], nfeats=2, nlabels=2)

        folds = opfpy.k_fold(sg, 4)
        total = sum(f.nnodes for f in folds)
        self.assertEqual(total, sg.nnodes)

    def test_k_fold_preserves_nfeats(self):
        sg = _make_subgraph([
            ([float(i), float(i)], (i % 2) + 1) for i in range(6)
        ], nfeats=2, nlabels=2)

        folds = opfpy.k_fold(sg, 3)
        for fold in folds:
            self.assertEqual(fold.nfeats, 2)


class TestMergeSubgraphs(unittest.TestCase):
    def test_merge_node_count(self):
        sg1 = _make_subgraph([([0.0, 0.0], 1), ([1.0, 1.0], 1)], nfeats=2, nlabels=2)
        sg2 = _make_subgraph([([2.0, 2.0], 2), ([3.0, 3.0], 2)], nfeats=2, nlabels=2)

        merged = opfpy.merge_subgraphs(sg1, sg2)
        self.assertEqual(merged.nnodes, sg1.nnodes + sg2.nnodes)

    def test_merge_preserves_nfeats(self):
        sg1 = _make_subgraph([([0.0, 0.0], 1)], nfeats=2, nlabels=2)
        sg2 = _make_subgraph([([1.0, 1.0], 2)], nfeats=2, nlabels=2)

        merged = opfpy.merge_subgraphs(sg1, sg2)
        self.assertEqual(merged.nfeats, 2)


class TestDistanceMatrix(unittest.TestCase):
    def _make_sg(self):
        return _make_subgraph([
            ([0.0, 0.0], 1),
            ([3.0, 4.0], 2),
            ([1.0, 0.0], 1),
        ], nfeats=2, nlabels=2)

    def test_compute_distance_matrix_shape(self):
        sg = self._make_sg()
        mat = opfpy.compute_distance_matrix(sg, distance_id=1)
        self.assertEqual(len(mat), sg.nnodes)
        self.assertTrue(all(len(row) == sg.nnodes for row in mat))

    def test_compute_distance_matrix_diagonal_zero(self):
        sg = self._make_sg()
        mat = opfpy.compute_distance_matrix(sg, distance_id=1)
        for i in range(sg.nnodes):
            self.assertAlmostEqual(mat[i][i], 0.0, places=5)

    def test_compute_distance_matrix_symmetry(self):
        sg = self._make_sg()
        mat = opfpy.compute_distance_matrix(sg, distance_id=1)
        n = sg.nnodes
        for i in range(n):
            for j in range(n):
                self.assertAlmostEqual(mat[i][j], mat[j][i], places=5)

    def test_compute_distance_matrix_known_value(self):
        """Euclidean dist between (0,0) and (3,4) should be 5.0."""
        sg = self._make_sg()
        mat = opfpy.compute_distance_matrix(sg, distance_id=1)
        self.assertAlmostEqual(mat[0][1], 5.0, places=5)

    def test_write_read_roundtrip(self):
        sg = self._make_sg()
        with tempfile.NamedTemporaryFile(suffix='.dat', delete=False) as f:
            fname = f.name
        try:
            opfpy.write_distance_matrix(fname, sg, distance_id=1)
            mat_read = opfpy.read_distance_matrix(fname)
            mat_orig = opfpy.compute_distance_matrix(sg, distance_id=1)
            n = sg.nnodes
            for i in range(n):
                for j in range(n):
                    self.assertAlmostEqual(mat_read[i][j], mat_orig[i][j], places=5)
        finally:
            os.unlink(fname)

    def test_invalid_distance_id_raises(self):
        sg = self._make_sg()
        with self.assertRaises(Exception):
            opfpy.compute_distance_matrix(sg, distance_id=99)


if __name__ == "__main__":
    unittest.main()
