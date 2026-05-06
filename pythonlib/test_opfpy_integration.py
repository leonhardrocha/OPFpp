"""
test_opfpy_integration.py — Phase 6 end-to-end integration tests.

Each test mirrors one of the example scripts (example1–6) and runs a
complete workflow on a real dataset file from ../data/.  The tests skip
gracefully when the data files are absent so CI never hard-fails on a
missing asset.

All core logic is called through the high-level `opf` package, which in
turn delegates to the `opfpy` C++ extension.
"""

import os
import sys
import math
import tempfile
import unittest

# Make the built extension importable
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "bin"))
# Make the opf package importable from this directory
sys.path.insert(0, os.path.dirname(__file__))

from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy

# Paths relative to this file
_HERE = os.path.dirname(__file__)
_DATA = os.path.normpath(os.path.join(_HERE, "..", "data"))

BOAT_DAT        = os.path.join(_DATA, "boat.dat")
CONE_TORUS_DAT  = os.path.join(_DATA, "cone-torus.dat")
DATA1_DAT       = os.path.join(_DATA, "data1.dat")
SATURN_DAT      = os.path.join(_DATA, "saturn.dat")


def _skip_if_missing(*paths):
    for p in paths:
        if not os.path.isfile(p):
            return unittest.skip(f"Data file not found: {p}")
    return lambda f: f


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _load(path):
    return opfpy.Subgraph.from_original_file(path)

def _split(sg, pct):
    first, second = opfpy.split_subgraph(sg, pct)
    return first, second

def _accuracy(sg):
    return opfpy.OPF().accuracy(sg)


# ---------------------------------------------------------------------------
# Example 1 — Supervised OPF without learning
# ---------------------------------------------------------------------------

class TestIntegrationExample1Supervised(unittest.TestCase):

    @_skip_if_missing(BOAT_DAT)
    def test_train_and_classify(self):
        """Example 1: train on 50%, classify the other 50%. Accuracy > 0."""
        from opf.supervised import train_and_classify
        from opf.utils import load, split

        data = load(BOAT_DAT)
        train_sg, test_sg = split(data, 0.5)

        acc = train_and_classify(train_sg, test_sg)

        self.assertIsInstance(acc, float)
        self.assertGreater(acc, 0.0)
        self.assertLessEqual(acc, 1.0)


# ---------------------------------------------------------------------------
# Example 2 — Supervised OPF with learning
# ---------------------------------------------------------------------------

class TestIntegrationExample2Learning(unittest.TestCase):

    @_skip_if_missing(BOAT_DAT)
    def test_learn_and_classify(self):
        """Example 2: 30/20/50 split, learning, classify. Accuracy > 0."""
        from opf.supervised import learn_and_classify
        from opf.utils import load, split

        data = load(BOAT_DAT)
        train_sg, rest = split(data, 0.3)
        eval_sg, test_sg = split(rest, 0.4)

        acc = learn_and_classify(train_sg, eval_sg, test_sg, n_iterations=5)

        self.assertIsInstance(acc, float)
        self.assertGreater(acc, 0.0)
        self.assertLessEqual(acc, 1.0)


# ---------------------------------------------------------------------------
# Example 3 — Precomputed distance matrix I/O
# ---------------------------------------------------------------------------

class TestIntegrationExample3DistanceMatrix(unittest.TestCase):

    @_skip_if_missing(CONE_TORUS_DAT)
    def test_distance_matrix_roundtrip_and_classify(self):
        """Example 3: compute Manhattan matrix, roundtrip binary I/O, classify."""
        from opf.utils import load, split, compute_distance_matrix
        from opf.utils import write_distance_matrix, read_distance_matrix
        from opf.supervised import train_and_classify
        from opf.distance import DistanceMetric

        data = load(CONE_TORUS_DAT)

        # Accept string, enum, and legacy int — all must resolve to the same result
        m_str   = compute_distance_matrix(data, "manhattan")
        m_enum  = compute_distance_matrix(data, DistanceMetric.MANHATTAN)
        m_int   = compute_distance_matrix(data, 3)
        self.assertAlmostEqual(m_str[0][1], m_enum[0][1], places=5)
        self.assertAlmostEqual(m_str[0][1], m_int[0][1],  places=5)

        matrix = m_enum
        n = data.nnodes
        self.assertEqual(len(matrix), n)
        self.assertEqual(len(matrix[0]), n)
        # Diagonal must be zero
        for i in range(n):
            self.assertAlmostEqual(matrix[i][i], 0.0, places=5)

        # Binary I/O roundtrip — use mkstemp to avoid Windows file-handle locking
        fd, dist_path = tempfile.mkstemp(suffix=".dist")
        os.close(fd)
        try:
            write_distance_matrix(matrix, dist_path)
            matrix2 = read_distance_matrix(dist_path)
            self.assertEqual(len(matrix2), n)
            self.assertAlmostEqual(matrix2[0][1], matrix[0][1], places=4)
        finally:
            os.unlink(dist_path)

        # Supervised classify still works
        train_sg, test_sg = split(data, 0.5)
        acc = train_and_classify(train_sg, test_sg)
        self.assertGreater(acc, 0.0)


# ---------------------------------------------------------------------------
# Example 4 — Feature normalization + supervised OPF
# ---------------------------------------------------------------------------

class TestIntegrationExample4Normalization(unittest.TestCase):

    @_skip_if_missing(BOAT_DAT)
    def test_normalize_then_classify(self):
        """Example 4: z-score normalize, then train/classify. Accuracy > 0."""
        from opf.utils import load, split, normalize
        from opf.supervised import train_and_classify

        data = load(BOAT_DAT)
        normalize(data)

        # Verify roughly zero mean per feature after normalization
        n = data.nnodes
        nfeats = data.nfeats
        for f in range(nfeats):
            mean = sum(data.get_node(i).feat[f] for i in range(n)) / n
            self.assertAlmostEqual(mean, 0.0, places=4,
                                   msg=f"Feature {f} mean not ~0 after normalization")

        train_sg, test_sg = split(data, 0.5)
        acc = train_and_classify(train_sg, test_sg)
        self.assertGreater(acc, 0.0)


# ---------------------------------------------------------------------------
# Example 5 — Unsupervised OPF (k-NN classify after cluster propagation)
# ---------------------------------------------------------------------------

class TestIntegrationExample5Unsupervised(unittest.TestCase):

    @_skip_if_missing(DATA1_DAT)
    def test_knn_classify_after_propagation(self):
        """Example 5: build knn radius, propagate labels, knn_classify test."""
        from opf.utils import load, split, accuracy

        data = load(DATA1_DAT)
        train_sg, test_sg = split(data, 0.8)

        k = 5
        n = train_sg.nnodes
        for i in range(n):
            fi = train_sg.get_node(i).feat
            dists = sorted(
                opfpy.eucl_dist(fi, train_sg.get_node(j).feat)
                for j in range(n) if j != i
            )
            train_sg.get_node(i).radius = dists[min(k - 1, len(dists) - 1)]

        # Propagate truelabel of root to tree members
        opfpy.propagate_cluster_labels(train_sg)

        clf = opfpy.OPF()
        clf.knn_classify(train_sg, test_sg)

        acc = accuracy(test_sg)
        self.assertIsInstance(acc, float)
        self.assertGreaterEqual(acc, 0.0)
        self.assertLessEqual(acc, 1.0)


# ---------------------------------------------------------------------------
# Example 6 — Semi-supervised OPF
# ---------------------------------------------------------------------------

class TestIntegrationExample6SemiSupervised(unittest.TestCase):

    @_skip_if_missing(SATURN_DAT)
    def test_semi_supervised_classify(self):
        """Example 6: labeled + unlabeled training, classify held-out test."""
        from opf.utils import load, split, accuracy
        from opf.unsupervised import semi_supervised
        from opf.supervised import classify as opf_classify

        data = load(SATURN_DAT)
        z1, rest = split(data, 0.6)
        _, test_sg = split(rest, 0.5)
        labeled_sg, unlabeled_sg = split(z1, 0.4)

        merged_sg = semi_supervised(labeled_sg, unlabeled_sg)

        self.assertGreater(merged_sg.nnodes, 0)

        opf_classify(merged_sg, test_sg)
        acc = accuracy(test_sg)

        self.assertIsInstance(acc, float)
        self.assertGreaterEqual(acc, 0.0)
        self.assertLessEqual(acc, 1.0)


# ---------------------------------------------------------------------------
# opf package import smoke-test
# ---------------------------------------------------------------------------

class TestPackageImports(unittest.TestCase):

    def test_import_opf_package(self):
        """The opf package must be importable and expose sub-modules."""
        import opf  # noqa: F401

    def test_import_opf_utils(self):
        from opf import utils  # noqa: F401

    def test_import_opf_supervised(self):
        from opf import supervised  # noqa: F401

    def test_import_opf_unsupervised(self):
        from opf import unsupervised  # noqa: F401

    def test_import_opf_distance_module(self):
        from opf import distance  # noqa: F401

    def test_distance_metric_enum(self):
        from opf.distance import DistanceMetric, resolve
        self.assertEqual(resolve("euclidean"),  1)
        self.assertEqual(resolve("MANHATTAN"),  3)
        self.assertEqual(resolve("l1"),         3)
        self.assertEqual(resolve("L2"),         1)
        self.assertEqual(resolve("cityblock"),  3)
        self.assertEqual(resolve(DistanceMetric.CANBERRA), 4)
        self.assertEqual(resolve(7),            7)

    def test_distance_metric_errors(self):
        from opf.distance import resolve
        with self.assertRaises(ValueError):
            resolve("no_such_metric")
        with self.assertRaises(TypeError):
            resolve(3.14)

    def test_distance_register(self):
        from opf.distance import register, resolve
        register("alias_for_bray", 7)
        self.assertEqual(resolve("alias_for_bray"), 7)

    def test_pretty_repr_node(self):
        import opf  # noqa: F401  # ensure repr patch side-effects are applied

        node = opfpy.Node()
        node.label = 2
        node.truelabel = 2
        node.pathval = 0.123456
        node.dens = 0.7
        node.radius = 1.2
        node.root = 10
        node.pred = 8
        node.position = 42
        node.status = 1
        node.relevant = 1
        node.nplatadj = 0
        node.feat = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7]
        node.adj = [1, 2, 3, 4, 5, 6, 7, 8]

        text = repr(node)
        self.assertIn("Node(", text)
        self.assertIn("label=2", text)
        self.assertIn("feat=[0.1, 0.2, 0.3, ..., 0.6, 0.7] (7)", text)
        self.assertIn("adj=[1, 2, 3, ..., 7, 8] (8)", text)

    def test_pretty_repr_subgraph_and_opf(self):
        import opf  # noqa: F401  # ensure repr patch side-effects are applied

        sg = opfpy.Subgraph()
        sg.nfeats = 2
        sg.nlabels = 3

        for i in range(6):
            node = opfpy.Node()
            node.label = (i % 3) + 1
            node.truelabel = (i % 3) + 1
            node.feat = [float(i), float(i + 1)]
            node.adj = [j for j in range(10)]
            sg.add_node(node)

        sg_text = repr(sg)
        self.assertIn("Subgraph(nnodes=6", sg_text)
        self.assertIn("[0] Node(", sg_text)
        self.assertIn("...", sg_text)
        self.assertIn("[5] Node(", sg_text)

        self.assertEqual(repr(opfpy.OPF()), "OPF()")


if __name__ == "__main__":
    unittest.main()
