#!/usr/bin/env python
"""
Phase 7: Multi-type Node/Subgraph support tests.
Validates that the template-based distance functions and multi-type bindings work correctly.
"""
import opfpy
import unittest


class TestPhase7MultiType(unittest.TestCase):
    """Test multi-type support for Node<T>, Subgraph<T>, and distance functions."""

    def test_float_distance_functions(self):
        """Float distance functions should still work."""
        v1 = [1.0, 2.0, 3.0]
        v2 = [4.0, 5.0, 6.0]
        
        # All distance functions should work with float vectors
        dist = opfpy.eucl_dist(v1, v2)
        self.assertAlmostEqual(dist, 5.196152422706632, places=5)
        
    def test_double_distance_functions(self):
        """Double distance functions should be available and work correctly."""
        v1 = [1.0, 2.0, 3.0]
        v2 = [4.0, 5.0, 6.0]
        
        # All distance functions should have double specializations
        dist = opfpy.eucl_dist_double(v1, v2)
        self.assertAlmostEqual(dist, 5.196152422706632, places=5)
        
    def test_node_float_dtype(self):
        """Node<float> should report dtype as 'float'."""
        n = opfpy.Node()
        self.assertEqual(n.dtype(), "float")
        
    def test_node_double_dtype(self):
        """Node<double> (NodeDouble) should report dtype as 'double'."""
        n = opfpy.NodeDouble()
        self.assertEqual(n.dtype(), "double")
        
    def test_subgraph_float_dtype(self):
        """Subgraph<float> should report dtype as 'float'."""
        sg = opfpy.Subgraph()
        self.assertEqual(sg.dtype(), "float")
        
    def test_subgraph_double_dtype(self):
        """Subgraph<double> (SubgraphDouble) should report dtype as 'double'."""
        sg = opfpy.SubgraphDouble()
        self.assertEqual(sg.dtype(), "double")
        
    def test_node_float_basic_properties(self):
        """Node<float> should support basic property access."""
        n = opfpy.Node()
        n.pathval = 1.5
        n.label = 0
        self.assertAlmostEqual(n.pathval, 1.5, places=5)
        self.assertEqual(n.label, 0)
        
    def test_node_double_basic_properties(self):
        """Node<double> should support basic property access."""
        n = opfpy.NodeDouble()
        n.pathval = 2.5
        n.label = 1
        self.assertAlmostEqual(n.pathval, 2.5, places=5)
        self.assertEqual(n.label, 1)
        
    def test_subgraph_float_node_access(self):
        """Subgraph<float> should allow node creation and access."""
        sg = opfpy.Subgraph()
        n = opfpy.Node()
        n.pathval = 0.0
        n.label = 1
        sg.add_node(n)
        self.assertEqual(sg.nnodes, 1)
        
    def test_subgraph_double_node_access(self):
        """Subgraph<double> should allow double node creation and access."""
        sg = opfpy.SubgraphDouble()
        n = opfpy.NodeDouble()
        n.pathval = 0.0
        n.label = 1
        sg.add_node(n)
        self.assertEqual(sg.nnodes, 1)
        
    def test_all_double_distance_functions_available(self):
        """All distance function variants should be available for double."""
        v1 = [1.0, 2.0, 3.0]
        v2 = [4.0, 5.0, 6.0]
        
        # Test all 7 distance function specializations
        self.assertGreater(opfpy.eucl_dist_double(v1, v2), 0)
        self.assertGreater(opfpy.chi_squared_dist_double(v1, v2), 0)
        self.assertGreater(opfpy.manhattan_dist_double(v1, v2), 0)
        self.assertGreater(opfpy.canberra_dist_double(v1, v2), 0)
        self.assertGreater(opfpy.squared_chord_dist_double(v1, v2), 0)
        self.assertGreater(opfpy.squared_chi_squared_dist_double(v1, v2), 0)
        self.assertGreater(opfpy.bray_curtis_dist_double(v1, v2), 0)


if __name__ == "__main__":
    unittest.main()
