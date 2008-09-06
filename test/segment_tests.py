import unittest
from libbless import *

class SegmentTests(unittest.TestCase):

	def setUp(self):
		self.abc = "abc"
		(self.error, self.seg) = segment_new(self.abc)
	
	def tearDown(self):
		segment_free(self.seg)

	def testNew(self):
		"Create a segment"
		self.assertNotEqual(self.seg, None)
		self.assertEqual(segment_get_size(self.seg)[1], 0)
		self.assert_(segment_get_data(self.seg)[1] is self.abc)

	def testClear(self):
		"Clear a segment"

		self.testChange()

		segment_clear(self.seg)
		self.assertEqual(segment_get_size(self.seg)[1], 0)

	def testChange(self):
		"Change a segment"
		segment_change(self.seg, 666, 112358)
		
		self.assertEqual(segment_get_start(self.seg)[1], 666)
		self.assertEqual(segment_get_end(self.seg)[1], 112358)

		self.assertEqual(segment_get_size(self.seg)[1], 112358 - 666 + 1)

	def testChangeInvalid(self):
		"Try to change a segment with start > end"
		segment_change(self.seg, 666, 112358)

		err = segment_change(self.seg, 555, 554)

		self.assertNotEqual(err, 0)
		
		# Make sure segment hasn't changed
		self.assertEqual(segment_get_start(self.seg)[1], 666)
		self.assertEqual(segment_get_end(self.seg)[1], 112358)

		self.assertEqual(segment_get_size(self.seg)[1], 112358 - 666 + 1)

	def testSplit(self):
		"Split a segment in the middle"
		segment_change(self.seg, 0, 999)

		(err, seg1) = segment_split(self.seg, 99)
		
		self.assertEqual(segment_get_start(self.seg)[1], 0)
		self.assertEqual(segment_get_end(self.seg)[1], 98)

		self.assertEqual(segment_get_start(seg1)[1], 99)
		self.assertEqual(segment_get_end(seg1)[1], 999)
		
		segment_free(seg1)

	def testSplit1(self):
		"Split a segment at the end"
		segment_change(self.seg, 0, 999)

		(err, seg1) = segment_split(self.seg, 999)
		
		self.assertEqual(segment_get_start(self.seg)[1], 0)
		self.assertEqual(segment_get_end(self.seg)[1], 998)

		self.assertEqual(segment_get_start(seg1)[1], 999)
		self.assertEqual(segment_get_end(seg1)[1], 999)
		
		segment_free(seg1)

	def testSplit2(self):
		"Split a segment at the start"
		segment_change(self.seg, 0, 999)

		(err, seg1) = segment_split(self.seg, 0)
		
		self.assertEqual(segment_get_size(self.seg)[1], 0)

		self.assertEqual(segment_get_start(seg1)[1], 0)
		self.assertEqual(segment_get_end(seg1)[1], 999)
		
		segment_free(seg1)

	def testSplitOutOfRange(self):
		"Try to split at a point out of range"
		segment_change(self.seg, 0, 999)

		(err, seg1) = segment_split(self.seg, 1000)
		
		self.assertNotEqual(err, 0)
		self.assertEqual(seg1, None)


if __name__ == '__main__':
	unittest.main()
