import unittest
import os
import sys

# Ensure the built extension is on the path when run from pythonlib/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'bin'))

# Add MSYS2/UCRT64 runtime DLL directories on Windows
from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy


class TestOPFPySupervised(unittest.TestCase):
    def _make_subgraph(self, samples, nfeats=2, nlabels=2):
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
        return sg

    def test_split_subgraph(self):
        original = self._make_subgraph([
            ([0.0, 0.0], 1),
            ([0.2, 0.1], 1),
            ([10.0, 10.0], 2),
            ([10.2, 9.9], 2),
        ])

        first, second = opfpy.split_subgraph(original, 0.5)

        self.assertEqual(first.nfeats, original.nfeats)
        self.assertEqual(second.nfeats, original.nfeats)
        self.assertEqual(first.nlabels, original.nlabels)
        self.assertEqual(second.nlabels, original.nlabels)
        self.assertEqual(first.nnodes + second.nnodes, original.nnodes)
        self.assertGreater(first.nnodes, 0)
        self.assertGreater(second.nnodes, 0)

        first, second = opfpy.split_subgraph(original, 0.8)

        self.assertEqual(first.nfeats, original.nfeats)
        self.assertEqual(second.nfeats, original.nfeats)
        self.assertEqual(first.nlabels, original.nlabels)
        self.assertEqual(second.nlabels, original.nlabels)
        self.assertEqual(first.nnodes + second.nnodes, original.nnodes)
        self.assertGreater(first.nnodes, 0)
        self.assertGreater(second.nnodes, 0)

    def test_train_classify_accuracy(self):
        train = self._make_subgraph([
            ([0.0, 0.0], 1),
            ([0.3, 0.2], 1),
            ([9.7, 10.1], 2),
            ([10.2, 9.9], 2),
        ])
        test = self._make_subgraph([
            ([0.1, 0.0], 1),
            ([0.4, 0.3], 1),
            ([9.8, 10.2], 2),
            ([10.1, 10.0], 2),
        ])

        clf = opfpy.OPF()
        clf.train(train)
        clf.classify(train, test)
        acc = clf.accuracy(test)

        self.assertGreaterEqual(acc, 0.99)
        labels = [test.get_node(i).label for i in range(test.nnodes)]
        self.assertTrue(all(label in (1, 2) for label in labels))

    def test_learn(self):
        train = self._make_subgraph([
            ([0.0, 0.1], 1),
            ([0.2, -0.1], 1),
            ([10.0, 9.9], 2),
            ([10.3, 10.2], 2),
        ])
        eval_sg = self._make_subgraph([
            ([0.05, 0.0], 1),
            ([10.1, 10.0], 2),
        ])

        clf = opfpy.OPF()
        clf.learn(train, eval_sg, 3)
        clf.classify(train, eval_sg)
        acc = clf.accuracy(eval_sg)

        self.assertGreaterEqual(acc, 0.0)
        self.assertLessEqual(acc, 1.0)


if __name__ == "__main__":
    unittest.main()
