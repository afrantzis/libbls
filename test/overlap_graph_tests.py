import unittest
import os
from StringIO import StringIO
from libbls import *

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

	def check_removed_edges(self, g, expected_lines):
		"""Check that the removed edges of graph g contain exactly the
		lines provided"""

		# Open a pipe to read the produced dot data
		(rfd, wfd) = os.pipe()
		(err, edges) = overlap_graph_get_removed_edges(g)
		self.assertEqual(err, 0)
		print_edge_list(edges, wfd)
		edge_str = os.read(rfd, 1000)

		# Make sure all lines exist in the dot file and remove them
		for line in expected_lines:
			self.assertNotEqual(edge_str.find(line), -1, "Cannot find '%s'"% line)
			edge_str = edge_str.replace(line, "", 1)

		# Check if anything remains (it should not)
		self.assertEqual(edge_str, "")

	def check_vertices_topo(self, g, expected_lines):
		"""Check that the topologically sorted vertices of graph g 
		are exactly the lines provided"""

		# Open a pipe to read the produced dot data
		(rfd, wfd) = os.pipe()
		(err, vertices) = overlap_graph_get_vertices_topo(g)
		self.assertEqual(err, 0)
		print_vertex_list(vertices, wfd)
		vertices_str = os.read(rfd, 1000)

		i = 0
		for line in StringIO(vertices_str):
			self.assertEqual(i < len(expected_lines), True)
			self.assertEqual(line, expected_lines[i])
			i = i + 1

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

		expected_lines = ("0 [label = \"0-0/0\"]\n", "1 [label = \"1-0/0\"]\n",
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

		expected_lines =("0 [label = \"0-1/1\"]\n", "1 [label = \"1-1/1\"]\n",
				"2 [label = \"2-1/1\"]\n", "0 -> 0 [label = 3]\n",
				"0 -> 2 [label = 3]\n", "1 -> 0 [label = 2]\n",
				"2 -> 1 [label = 3]\n")

		self.check_dot(self.g, expected_lines)

	def testSpanningTree(self):
		"Find the spanning tree of an overlap graph"

		self.testAddSegments2()

		overlap_graph_remove_cycles(self.g)

		expected_lines = ("0 [label = \"0-0/1\"]\n", "1 [label = \"1-1/0\"]\n",
				"2 [label = \"2-1/1\"]\n", 
				"0 -> 0 [label = 3]\n",
				"0 -> 2 [label = 3]\n",
				"1 -> 0 [label = 2 style = dotted]\n",
				"2 -> 1 [label = 3]\n")

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

		expected_lines = ("0 [label = \"0-0/2\"]\n", "1 [label = \"1-1/1\"]\n",
				"2 [label = \"2-2/0\"]\n",
				"0 -> 1 [label = 5]\n", "0 -> 2 [label = 3]\n",
				"1 -> 2 [label = 2]\n") 

		self.check_dot(self.g, expected_lines)

		overlap_graph_remove_cycles(self.g)

		expected_lines = ("0 [label = \"0-0/2\"]\n", "1 [label = \"1-1/1\"]\n",
				"2 [label = \"2-2/0\"]\n",
				"0 -> 1 [label = 5]\n", 
				"0 -> 2 [label = 3]\n",
				"1 -> 2 [label = 2]\n") 

		self.check_dot(self.g, expected_lines)

	def testGetRemovedEdges(self):
		"Get the removed edges of the graph"

		(err, seg1) = segment_new("A1", 5, 10, None)
		self.assertEqual(err, 0)
		(err, seg2) = segment_new("B2", 20, 5, None)
		self.assertEqual(err, 0)
		(err, seg3) = segment_new("C3", 30, 5, None)
		self.assertEqual(err, 0)

		err = overlap_graph_add_segment(self.g, seg1, 12)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg2, 28)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg3, 3)
		self.assertEqual(err, 0)

		# No removed edges so far
		self.check_removed_edges(self.g, [])

		overlap_graph_remove_cycles(self.g)

		# Check removed edges after removing cycles
		expected_edges = ["%s -> %s\n" % (segment_get_data(seg2)[1],
				segment_get_data(seg1)[1])]

		self.check_removed_edges(self.g, expected_edges)

	def testTopo(self):
		"Get the vertices of the graph in topological order"

		(err, seg1) = segment_new("A1", 5, 10, None)
		self.assertEqual(err, 0)
		(err, seg2) = segment_new("B2", 20, 5, None)
		self.assertEqual(err, 0)
		(err, seg3) = segment_new("C3", 30, 5, None)
		self.assertEqual(err, 0)

		err = overlap_graph_add_segment(self.g, seg1, 12)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg2, 28)
		self.assertEqual(err, 0)
		err = overlap_graph_add_segment(self.g, seg3, 3)
		self.assertEqual(err, 0)

		overlap_graph_remove_cycles(self.g)

		# Check vertices  
		expected_vertices = ["%s\n" % segment_get_data(seg1)[1],
				"%s\n" % segment_get_data(seg3)[1],
				"%s\n" % segment_get_data(seg2)[1]]

		self.check_vertices_topo(self.g, expected_vertices)


if __name__ == '__main__':
	unittest.main()

