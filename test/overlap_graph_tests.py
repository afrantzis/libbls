import unittest
import os
from libbless import *

class OverlapGraphTests(unittest.TestCase):

	def check_dot(self, g, expected_lines):
		"""Check that the dot file of graph g contains exactly the
		lines provided"""

		# Open a pipe to read the produced dot data
		(rfd, wfd) = os.pipe()
		overlap_graph_export_dot(g, wfd)
		dot_str = os.read(rfd, 1000)

		# Make sure all lines exist in the dot file and remove them
		for line in expected_lines:
			self.assertNotEqual(dot_str.find(line), -1, "Cannot find '%s'"% line)
			dot_str = dot_str.replace(line, "", 1)

		# Remove dot header/footer
		dot_str = dot_str.replace("digraph overlap_graph {\n}\n", "", 1)

		# Check if anything remains (it should not)
		self.assertEqual(dot_str, "")

	def setUp(self):
		(err, self.g) = overlap_graph_new(1)
		self.assertEqual(err, 0)

	def tearDown(self):
		overlap_graph_free(self.g)

	def testAddSegments1(self):
		"Add segments to the overlap graph (1)" 

		(err, seg1) = segment_new("", 5, 10, None)
		self.assertEqual(err, 0)
		(err, seg2) = segment_new("", 20, 5, None)
		self.assertEqual(err, 0)

		err = overlap_graph_add_segment(self.g, seg1, 8)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg2, 18)
		self.assertEqual(err, 0)

		expected_lines = ("0 [label = 0]\n", "1 [label = 0]\n",
				"0 -> 0 [label = 7]\n", "1 -> 1 [label = 3]\n")

		self.check_dot(self.g, expected_lines)

	def testAddSegments2(self):
		"Add segments to the overlap graph (2)" 

		(err, seg1) = segment_new("", 5, 10, None)
		self.assertEqual(err, 0)
		(err, seg2) = segment_new("", 20, 5, None)
		self.assertEqual(err, 0)
		(err, seg3) = segment_new("", 30, 5, None)
		self.assertEqual(err, 0)

		err = overlap_graph_add_segment(self.g, seg1, 12)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg2, 28)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg3, 3)
		self.assertEqual(err, 0)

		expected_lines = ("0 [label = 1]\n", "1 [label = 1]\n",  "2 [label = 1]\n",
				"0 -> 0 [label = 3]\n", "0 -> 2 [label = 3]\n",
				"1 -> 0 [label = 2]\n", "2 -> 1 [label = 3]\n")

		self.check_dot(self.g, expected_lines)

	def testSpanningTree(self):
		"Find the spanning tree of an overlap graph"

		self.testAddSegments2()

		overlap_graph_max_spanning_tree(self.g)

		expected_lines = ("0 [label = 0]\n", "1 [label = 1]\n",  "2 [label = 1]\n",
				"0 -> 0 [label = 3]\n", "0 -> 2 [label = 3 style = bold]\n",
				"1 -> 0 [label = 2]\n", "2 -> 1 [label = 3 style = bold]\n")

		self.check_dot(self.g, expected_lines)

	def testSpanningTreeUndirectedCycle(self):
		"Find the spanning tree of an overlap graph with an undirected cycle"

		(err, seg1) = segment_new("", 5, 10, None)
		self.assertEqual(err, 0)
		(err, seg2) = segment_new("", 15, 5, None)
		self.assertEqual(err, 0)
		(err, seg3) = segment_new("", 40, 5, None)
		self.assertEqual(err, 0)

		err = overlap_graph_add_segment(self.g, seg1, 20)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg2, 5)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg3, 12)
		self.assertEqual(err, 0)

		expected_lines = ("0 [label = 0]\n", "1 [label = 1]\n",  "2 [label = 2]\n",
				"0 -> 1 [label = 5]\n", "0 -> 2 [label = 3]\n",
				"1 -> 2 [label = 2]\n") 

		self.check_dot(self.g, expected_lines)

		overlap_graph_max_spanning_tree(self.g)

		expected_lines = ("0 [label = 0]\n", "1 [label = 1]\n",  "2 [label = 1]\n",
				"0 -> 1 [label = 5 style = bold]\n", 
				"0 -> 2 [label = 3 style = bold]\n",
				"1 -> 2 [label = 2]\n") 

		self.check_dot(self.g, expected_lines)


if __name__ == '__main__':
	unittest.main()

