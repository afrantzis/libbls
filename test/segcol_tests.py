import unittest
from libbless import *

class SegcolTestsList(unittest.TestCase):

	def setUp(self):
		(err, self.segcol) = segcol_new("list")
		self.assertEqual(err, 0)

	def tearDown(self):
		segcol_free(self.segcol)

	def check_iter_segments(self, segs):
		"""Check, using an iterator, if the segments in the segcol match the ones 
		supplied by the user. The 'segs' arguments is a list of tuples. Each tuple 
		contains the expected values of each segment:
		    (data, mapping, start, end)"""

		iter = segcol_iter_new(self.segcol)[1]
		i = 0

		while segcol_iter_is_valid(iter)[1] == 1:
			seg_tmp = segcol_iter_get_segment(iter)[1]
			t = (segment_get_data(seg_tmp)[1], segcol_iter_get_mapping(iter)[1],
					segment_get_start(seg_tmp)[1], segment_get_end(seg_tmp)[1])
			self.assertEqual(t, segs[i])
	
			segcol_iter_next(iter)
			i = i + 1

		self.assertEqual(i, len(segs))
		segcol_iter_free(iter)
	
	def testNew(self):
		"Create a new segcol"

		self.assertNotEqual(self.segcol, None)
		self.assertEqual(segcol_get_size(self.segcol)[1], 0)

	def testAppend(self):
		"Append segments to the segcol"

		(err, seg1) = segment_new("abcdef")
		segment_change(seg1, 0, 6)

		(err, seg2) = segment_new("012345")
		segment_change(seg2, 0, 6)

		# Append segments and test segcol size
		segcol_append(self.segcol, seg1)
		self.assertEqual(segcol_get_size(self.segcol)[1], 6)
		
		segcol_append(self.segcol, seg2)
		self.assertEqual(segcol_get_size(self.segcol)[1], 12)

		# Segcol should be ["abcdef"]-["012345"]

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
	
	def testIterator(self):
		"Create and traverse an iterator"

		nseg = 10
		seg = []

		for i in xrange(nseg):
			(err, seg_tmp) = segment_new("%d%d" % (i, i))
			seg.append(("%d%d" % (i, i), 2 * i, 0, 1))
			segment_change(seg_tmp, 0, 2)
			segcol_append(self.segcol, seg_tmp)

		self.check_iter_segments(seg)
		
	def testInsertBeginning(self):
		"Insert a segment at the beginning of another segment"

		# Append some segments to the segcol
		self.testAppend()

		(err, seg1) = segment_new("BBB")
		segment_change(seg1, 0, 3)

		# Insert segment at the beginning of anothe segment
		self.assertEqual(segcol_insert(self.segcol, 0, seg1), 0)
		self.assertEqual(segcol_get_size(self.segcol)[1], 15)

		# Segcol should be ["BBB"]-["abcdef"]-["012345"]
		segs = [("BBB", 0, 0, 2), ("abcdef", 3, 0, 5), ("012345", 9, 0, 5)]

		self.check_iter_segments(segs)
		
	def testInsertEnd(self):
		"Insert a segment at the end of another segment"

		# Insert some segments into the segcol
		self.testInsertBeginning()

		(err, seg1) = segment_new("EEE")
		segment_change(seg1, 0, 3)

		# Insert segment at the end of another segment
		segcol_insert(self.segcol, 8, seg1)
		self.assertEqual(segcol_get_size(self.segcol)[1], 18)

		# Segcol should be ["BBB"]-["abcde"f]-["EEE"]-[abcde"f"]-["012345"]
		segs = [("BBB", 0, 0, 2), ("abcdef", 3, 0, 4), ("EEE", 8, 0, 2),
				("abcdef", 11, 5, 5), ("012345", 12, 0, 5)]

		self.check_iter_segments(segs)

	def testInsertMiddle(self):
		"Insert a segment in the middle of another segment"

		# Insert some segments into the segcol
		self.testInsertEnd()

		(err, seg1) = segment_new("MMM")
		segment_change(seg1, 0, 3)

		# Insert segment at the end of another segment
		segcol_insert(self.segcol, 5, seg1)
		self.assertEqual(segcol_get_size(self.segcol)[1], 21)

		# Segcol should be ["BBB"]-["ab"cdef]-["MMM"]-[ab"cde"f]-["EEE"]
		# -[abcde"f"]-["012345"]
		segs = [("BBB", 0, 0, 2), ("abcdef", 3, 0, 1), ("MMM", 5, 0, 2), 
				("abcdef", 8, 2, 4), ("EEE", 11, 0, 2), ("abcdef", 14, 5, 5),
				("012345", 15, 0, 5)]

		self.check_iter_segments(segs)

	def testTryInsertNegative(self):
		"Try to insert a segment at a negative index"

		(err, seg1) = segment_new("ERROR")
		segment_change(seg1, 0, 5)
		
		err = segcol_insert(self.segcol, -1, seg1)
		self.assertNotEqual(err, 0)

		# Insert some segments into the segcol
		self.testInsertBeginning()

		err = segcol_insert(self.segcol, -1, seg1)
		self.assertNotEqual(err, 0)

		# Segcol should remain ["BBB"]-["abcdef"]-["012345"]
		segs = [("BBB", 0, 0, 2), ("abcdef", 3, 0, 5), ("012345", 9, 0, 5)]

		self.check_iter_segments(segs)

	def testTryInsertBeyondEOF(self):
		"Try to insert a segment beyond the end of a buffer"

		(err, seg1) = segment_new("ERROR")
		segment_change(seg1, 0, 5)
		
		err = segcol_insert(self.segcol, 0, seg1)
		self.assertNotEqual(err, 0)

		# Insert some segments into the segcol
		self.testInsertBeginning()
		size = segcol_get_size(self.segcol)[1]

		err = segcol_insert(self.segcol, size, seg1)
		self.assertNotEqual(err, 0)

		# Segcol should remain ["BBB"]-["abcdef"]-["012345"]
		segs = [("BBB", 0, 0, 2), ("abcdef", 3, 0, 5), ("012345", 9, 0, 5)]

		self.check_iter_segments(segs)


if __name__ == '__main__':
	unittest.main()
