import unittest
from distance import euclidean_distance, chi_square_distance, compute_distance_matrix

class TestDistance(unittest.TestCase):
    def test_euclidean(self):
        a = [0.0, 0.0]
        b = [3.0, 4.0]
        self.assertAlmostEqual(euclidean_distance(a, b), 5.0)
        self.assertAlmostEqual(euclidean_distance(a, a), 0.0)

    def test_chi_square(self):
        a = [2.0, 2.0]
        b = [4.0, 6.0]
        # ((2-4)^2)/(2+4) + ((2-6)^2)/(2+6) = (4/6) + (16/8) = 0.666... + 2.0 = 2.666...
        self.assertAlmostEqual(chi_square_distance(a, b), 2.666666, places=5)
        self.assertAlmostEqual(chi_square_distance(a, a), 0.0)

    def test_distance_matrix(self):
        feats = [[0.0, 0.0], [3.0, 4.0], [6.0, 8.0]]
        mat = compute_distance_matrix(feats, metric='euclidean')
        self.assertAlmostEqual(mat[0][1], 5.0)
        self.assertAlmostEqual(mat[1][2], 5.0)
        self.assertAlmostEqual(mat[0][2], 10.0)
        self.assertAlmostEqual(mat[0][0], 0.0)
        self.assertAlmostEqual(mat[1][1], 0.0)
        self.assertAlmostEqual(mat[2][2], 0.0)

if __name__ == '__main__':
    unittest.main()
