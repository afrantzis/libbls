import unittest
import errno
from libbls import *

class SegmentTests(unittest.TestCase):

	def setUp(self):
		self.abc = "abc"
		(self.error, self.seg) = segment_new(self.abc, 0, 0, None)
	
	def tearDown(self):
		segment_free(self.seg)

	def testNew(self):
		"Create a segment"
		self.assertNotEqual(self.seg, None)
		self.assertEqual(segment_get_size(self.seg)[1], 0)
		self.assert_(segment_get_data(self.seg)[1] is self.abc)
	
	def testCopy(self):
		"Copy a segment"

		self.testChangeRange()

		(err, seg1) = segment_copy(self.seg)

		self.assertEqual(segment_get_start(seg1)[1],
				segment_get_start(self.seg)[1])

		self.assertEqual(segment_get_size(seg1)[1],
				segment_get_size(self.seg)[1])

		self.assert_(segment_get_data(seg1)[1] is
				segment_get_data(self.seg)[1])
		
	def testClear(self):
		"Clear a segment"
		self.testChangeRange()

		segment_clear(self.seg)
		self.assertEqual(segment_get_size(self.seg)[1], 0)

	def testChangeRange(self):
		"Change a segment"
		segment_set_range(self.seg, 666, 112358)
		
		self.assertEqual(segment_get_start(self.seg)[1], 666)

		self.assertEqual(segment_get_size(self.seg)[1], 112358)
	
	def testSplitMiddle(self):
		"Split a segment in the middle"
		segment_set_range(self.seg, 0, 1000)

		(err, seg1) = segment_split(self.seg, 99)
		
		self.assertEqual(segment_get_start(self.seg)[1], 0)
		self.assertEqual(segment_get_size(self.seg)[1], 99)

		self.assertEqual(segment_get_start(seg1)[1], 99)
		self.assertEqual(segment_get_size(seg1)[1], 901)
		
		segment_free(seg1)

	def testSplitEnd(self):
		"Split a segment at the end"
		segment_set_range(self.seg, 0, 1000)

		(err, seg1) = segment_split(self.seg, 999)
		
		self.assertEqual(segment_get_start(self.seg)[1], 0)
		self.assertEqual(segment_get_size(self.seg)[1], 999)

		self.assertEqual(segment_get_start(seg1)[1], 999)
		self.assertEqual(segment_get_size(seg1)[1], 1)
		
		segment_free(seg1)

	def testSplitStart(self):
		"Split a segment at the start"
		segment_set_range(self.seg, 0, 1000)

		(err, seg1) = segment_split(self.seg, 0)
		
		self.assertEqual(segment_get_size(self.seg)[1], 0)

		self.assertEqual(segment_get_start(seg1)[1], 0)
		self.assertEqual(segment_get_size(seg1)[1], 1000)
		
		segment_free(seg1)

	def testSplitOutOfRange(self):
		"Try to split at a point out of range"
		segment_set_range(self.seg, 0, 1000)

		(err, seg1) = segment_split(self.seg, 1000)
		
		self.assertNotEqual(err, 0)
		self.assertEqual(seg1, None)
		
	def testMerge(self):
		"Merge two segments"
		segment_set_range(self.seg, 0, 1000)

		(err, seg1) = segment_split(self.seg, 600)
		self.assertEqual(err, 0)

		self.assertEqual(segment_get_start(self.seg)[1], 0)
		self.assertEqual(segment_get_size(self.seg)[1], 600)

		self.assertEqual(segment_get_start(seg1)[1], 600)
		self.assertEqual(segment_get_size(seg1)[1], 400)

		# Merge the segments
		err = segment_merge(self.seg, seg1)
		self.assertEqual(err, 0)
		self.assertEqual(segment_get_start(self.seg)[1], 0)
		self.assertEqual(segment_get_size(self.seg)[1], 1000)

		self.assertEqual(segment_get_start(seg1)[1], 600)
		self.assertEqual(segment_get_size(seg1)[1], 400)
		
		segment_free(seg1)

	def testMergeBoundaryCases(self):
		"Try boundary conditions when merging"

		err = segment_set_range(self.seg, 0, 1000)
		self.assertEqual(err, 0)

		(err, seg1) = segment_copy(self.seg)
		self.assertEqual(err, 0)

		err = segment_set_range(self.seg, 1001, 10)
		self.assertEqual(err, 0)

		err = segment_merge(self.seg, seg1)
		self.assertNotEqual(err, 0)

		segment_free(seg1)

	def testMergeOverflow(self):
		"Try boundary conditions for overflow when merging"

		err = segment_set_range(self.seg, 0, 1)
		self.assertEqual(err, 0)

		(err, seg1) = segment_copy(self.seg)
		self.assertEqual(err, 0)

		err = segment_set_range(seg1, 1, get_max_off_t())
		self.assertEqual(err, 0)

		err = segment_merge(self.seg, seg1)
		self.assertEqual(err, errno.EOVERFLOW)

		# This should succeed
		err = segment_set_range(seg1, 1, get_max_off_t() - 1)
		self.assertEqual(err, 0)

		err = segment_merge(self.seg, seg1)
		self.assertEqual(err, 0)

		segment_free(seg1)

	def testRangeOverflow(self):
		"Try boundary conditions for overflow"

		# These should succeed
		err = segment_set_range(self.seg, 0, get_max_off_t()) 
		self.assertEqual(err, 0)

		err = segment_set_range(self.seg, get_max_off_t(), 0) 
		self.assertEqual(err, 0)

		err = segment_set_range(self.seg, 1, get_max_off_t()) 
		self.assertEqual(err, 0)

		err = segment_set_range(self.seg, get_max_off_t(), 1) 
		self.assertEqual(err, 0)

		# These should fail
		err = segment_set_range(self.seg, 2, get_max_off_t()) 
		self.assertNotEqual(err, 0)

		err = segment_set_range(self.seg, get_max_off_t(), 2) 
		self.assertNotEqual(err, 0)

if __name__ == '__main__':
	unittest.main()
