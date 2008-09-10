import unittest
from libbless import *

class SegcolTestsList(unittest.TestCase):

	def setUp(self):
		(err, self.segcol) = segcol_new("list")
		self.assertEqual(err, 0)

	def tearDown(self):
		segcol_free(self.segcol)

	def testNew(self):
		"Create a new segcol"
		self.assertNotEqual(self.segcol, None)
		self.assertEqual(segcol_get_size(self.segcol)[1], 0)

	def testAppend(self):
		"Append segments to the segcol"
		(err, seg1) = segment_new("abcdef")
		segment_change(seg1, 0, 5)

		(err, seg2) = segment_new("012345")
		segment_change(seg2, 0, 5)

		# Append segments and test segcol size
		segcol_append(self.segcol, seg1)
		self.assertEqual(segcol_get_size(self.segcol)[1], 6)
		
		segcol_append(self.segcol, seg2)
		self.assertEqual(segcol_get_size(self.segcol)[1], 12)

		# Check seg1 corner cases
		iter = segcol_find(self.segcol, 0)[1]
		seg = segcol_iter_get_segment(iter)[1]

		self.assertEqual(seg, seg1)

		iter = segcol_find(self.segcol, 5)[1]
		seg = segcol_iter_get_segment(iter)[1]

		self.assertEqual(seg, seg1)

		# Check seg2 corner cases
		iter = segcol_find(self.segcol, 6)[1]
		seg = segcol_iter_get_segment(iter)[1]

		self.assertEqual(seg, seg2)

		iter = segcol_find(self.segcol, 11)[1]
		seg = segcol_iter_get_segment(iter)[1]

		self.assertEqual(seg, seg2)
		
		

if __name__ == '__main__':
	unittest.main()
