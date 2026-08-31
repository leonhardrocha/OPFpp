import os
import sys
import unittest
import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "bin"))

from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy
from opfppy import (
	eucl_dist,
	manhattan_dist,
	DistanceMetric,
)
from opfppy.distance import _detect_precision, distance

class TestDistanceShimRuntimeDispatch(unittest.TestCase):
    
    def setUp(self):
        """Test Fixture: Configura os arrays do NumPy antes de cada teste."""
        # Arrays simples para detecção de tipo
        self.single_float_a = np.array([1.0], dtype=np.float32)
        self.single_float_b = np.array([2.0], dtype=np.float32)
        
        self.single_double_a = np.array([1.0], dtype=np.float64)
        self.single_double_b = np.array([2.0], dtype=np.float64)
        
        # Arrays de coordenadas para cálculo de distância
        self.coords_float_a = np.array([1.0, 2.0, 3.0], dtype=np.float32)
        self.coords_float_b = np.array([2.0, 4.0, 6.0], dtype=np.float32)
        
        self.coords_double_a = np.array([1.0, 2.0, 3.0], dtype=np.float64)
        self.coords_double_b = np.array([2.0, 4.0, 6.0], dtype=np.float64)

    def test_detect_precision_array_f(self):
        self.assertEqual(
            _detect_precision(self.single_float_a, self.single_float_b), 
            "float"
        )

    def test_detect_precision_array_d(self):
        self.assertEqual(
            _detect_precision(self.single_double_a, self.single_double_b), 
            "double"
        )

    def test_eucl_dist_float_dispatch(self):
        self.assertAlmostEqual(
            eucl_dist(self.coords_float_a, self.coords_float_b), 
            opfpy.eucl_dist(self.coords_float_a, self.coords_float_b), 
            places=6
        )

    def test_eucl_dist_double_dispatch(self):
        self.assertAlmostEqual(
            eucl_dist(self.coords_double_a, self.coords_double_b), 
            opfpy.eucl_dist_double(self.coords_double_a, self.coords_double_b), 
            places=12
        )

    def test_generic_distance_metric_dispatch(self):
        self.assertAlmostEqual(
            distance(self.coords_double_a, self.coords_double_b, metric=DistanceMetric.MANHATTAN),
            manhattan_dist(self.coords_double_a, self.coords_double_b),
            places=12,
        )



if __name__ == "__main__":
	unittest.main()
