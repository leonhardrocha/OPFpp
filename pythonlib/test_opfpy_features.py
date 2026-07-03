import os
import sys
import unittest
from array import array

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "bin"))

from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy
from opfppy import (
	eucl_dist,
	distance,
	DistanceMetric,
)
from opfppy.distance import _detect_precision


class TestDistanceShimRuntimeDispatch(unittest.TestCase):
	def test_detect_precision_array_f(self):
		self.assertEqual(_detect_precision(array("f", [1.0]), array("f", [2.0])), "float")

	def test_detect_precision_array_d(self):
		self.assertEqual(_detect_precision(array("d", [1.0]), array("d", [2.0])), "double")

	def test_eucl_dist_float_dispatch(self):
		a = array("f", [1.0, 2.0, 3.0])
		b = array("f", [2.0, 4.0, 6.0])
		self.assertAlmostEqual(eucl_dist(a, b), opfpy.eucl_dist(a, b), places=6)

	def test_eucl_dist_double_dispatch(self):
		a = array("d", [1.0, 2.0, 3.0])
		b = array("d", [2.0, 4.0, 6.0])
		self.assertAlmostEqual(eucl_dist(a, b), opfpy.eucl_dist_double(a, b), places=12)

	def test_generic_distance_metric_dispatch(self):
		a = array("d", [1.0, 2.0, 3.0])
		b = array("d", [2.0, 4.0, 6.0])
		self.assertAlmostEqual(
			distance(a, b, DistanceMetric.MANHATTAN),
			opfpy.manhattan_dist_double(a, b),
			places=12,
		)


if __name__ == "__main__":
	unittest.main()
