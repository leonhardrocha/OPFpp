
import unittest
import os
import sys
from array import array

# Ensure the built extension is on the path when run from pythonlib/
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'bin'))

# Add MSYS2/UCRT64 runtime DLL directories on Windows
from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

import opfpy
from opfppy import distance as dist_shim

class TestOPFPyDistance(unittest.TestCase):
    def setUp(self):
        self.v1 = [1.0, 2.0, 3.0]
        self.v2 = [4.0, 5.0, 6.0]
        self.v3 = [1.0, 2.0, 3.0]  # identical to v1
        self.empty = []
        self.v1_f = array("f", [1.0, 2.0, 3.0])
        self.v2_f = array("f", [4.0, 5.0, 6.0])
        self.v3_f = array("f", [1.0, 2.0, 3.0])
        self.v1_d = array("d", [1.0, 2.0, 3.0])
        self.v2_d = array("d", [4.0, 5.0, 6.0])
        self.v3_d = array("d", [1.0, 2.0, 3.0])

    def test_runtime_dispatch_single_and_double(self):
        self.assertAlmostEqual(dist_shim.eucl_dist(self.v1_f, self.v2_f), opfpy.eucl_dist(self.v1_f, self.v2_f), places=6)
        self.assertAlmostEqual(dist_shim.eucl_dist(self.v1_d, self.v2_d), opfpy.eucl_dist_double(self.v1_d, self.v2_d), places=12)

        self.assertAlmostEqual(dist_shim.chi_squared_dist(self.v1_f, self.v2_f), opfpy.chi_squared_dist(self.v1_f, self.v2_f), places=6)
        self.assertAlmostEqual(dist_shim.chi_squared_dist(self.v1_d, self.v2_d), opfpy.chi_squared_dist_double(self.v1_d, self.v2_d), places=12)

        self.assertAlmostEqual(dist_shim.manhattan_dist(self.v1_f, self.v2_f), opfpy.manhattan_dist(self.v1_f, self.v2_f), places=6)
        self.assertAlmostEqual(dist_shim.manhattan_dist(self.v1_d, self.v2_d), opfpy.manhattan_dist_double(self.v1_d, self.v2_d), places=12)

        self.assertAlmostEqual(dist_shim.canberra_dist(self.v1_f, self.v2_f), opfpy.canberra_dist(self.v1_f, self.v2_f), places=6)
        self.assertAlmostEqual(dist_shim.canberra_dist(self.v1_d, self.v2_d), opfpy.canberra_dist_double(self.v1_d, self.v2_d), places=12)

        self.assertAlmostEqual(dist_shim.squared_chord_dist(self.v1_f, self.v2_f), opfpy.squared_chord_dist(self.v1_f, self.v2_f), places=6)
        self.assertAlmostEqual(dist_shim.squared_chord_dist(self.v1_d, self.v2_d), opfpy.squared_chord_dist_double(self.v1_d, self.v2_d), places=12)

        self.assertAlmostEqual(dist_shim.squared_chi_squared_dist(self.v1_f, self.v2_f), opfpy.squared_chi_squared_dist(self.v1_f, self.v2_f), places=6)
        self.assertAlmostEqual(dist_shim.squared_chi_squared_dist(self.v1_d, self.v2_d), opfpy.squared_chi_squared_dist_double(self.v1_d, self.v2_d), places=12)

        self.assertAlmostEqual(dist_shim.bray_curtis_dist(self.v1_f, self.v2_f), opfpy.bray_curtis_dist(self.v1_f, self.v2_f), places=6)
        self.assertAlmostEqual(dist_shim.bray_curtis_dist(self.v1_d, self.v2_d), opfpy.bray_curtis_dist_double(self.v1_d, self.v2_d), places=12)

        self.assertEqual(dist_shim.eucl_dist(self.v1_f, self.v3_f), 0.0)
        self.assertEqual(dist_shim.eucl_dist(self.v1_d, self.v3_d), 0.0)

    def test_eucl_dist(self):
        self.assertAlmostEqual(opfpy.eucl_dist(self.v1, self.v2), 5.196152, places=5)
        self.assertEqual(opfpy.eucl_dist(self.v1, self.v3), 0.0)

    def test_chi_squared_dist(self):
        d = opfpy.chi_squared_dist(self.v1, self.v2)
        self.assertTrue(d > 0)
        self.assertEqual(opfpy.chi_squared_dist(self.v1, self.v3), 0.0)

    def test_manhattan_dist(self):
        self.assertEqual(opfpy.manhattan_dist(self.v1, self.v2), 9.0)
        self.assertEqual(opfpy.manhattan_dist(self.v1, self.v3), 0.0)

    def test_canberra_dist(self):
        d = opfpy.canberra_dist(self.v1, self.v2)
        self.assertTrue(d > 0)
        self.assertEqual(opfpy.canberra_dist(self.v1, self.v3), 0.0)

    def test_squared_chord_dist(self):
        d = opfpy.squared_chord_dist(self.v1, self.v2)
        self.assertTrue(d > 0)
        self.assertEqual(opfpy.squared_chord_dist(self.v1, self.v3), 0.0)

    def test_squared_chi_squared_dist(self):
        d = opfpy.squared_chi_squared_dist(self.v1, self.v2)
        self.assertTrue(d > 0)
        self.assertEqual(opfpy.squared_chi_squared_dist(self.v1, self.v3), 0.0)

    def test_bray_curtis_dist(self):
        d = opfpy.bray_curtis_dist(self.v1, self.v2)
        self.assertTrue(d > 0)
        self.assertEqual(opfpy.bray_curtis_dist(self.v1, self.v3), 0.0)

    def test_empty_vectors(self):
        self.assertEqual(opfpy.eucl_dist(self.empty, self.empty), 0.0)
        self.assertEqual(opfpy.manhattan_dist(self.empty, self.empty), 0.0)
        self.assertEqual(opfpy.canberra_dist(self.empty, self.empty), 0.0)
        self.assertEqual(opfpy.squared_chord_dist(self.empty, self.empty), 0.0)
        self.assertEqual(opfpy.squared_chi_squared_dist(self.empty, self.empty), 0.0)
        self.assertEqual(opfpy.bray_curtis_dist(self.empty, self.empty), 0.0)

if __name__ == "__main__":
    unittest.main()
