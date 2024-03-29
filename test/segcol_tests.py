import unittest
from libbls import *

class SegcolTestsList(unittest.TestCase):

	def setUp(self):
		(err, self.segcol) = segcol_list_new()
		self.assertEqual(err, 0)

	def tearDown(self):
		segcol_free(self.segcol)

	def check_iter_segments(self, segcol, segs):
		"""Check, using an iterator, if the segments in the segcol match the ones 
		supplied by the user. The 'segs' arguments is a list of tuples. Each tuple 
		contains the expected values of each segment:
		    (data, mapping, start, size)"""

		iter = segcol_iter_new(segcol)[1]
		i = 0

		while segcol_iter_is_valid(iter)[1] == 1:
			seg_tmp = segcol_iter_get_segment(iter)[1]
			t = (segment_get_data(seg_tmp)[1], segcol_iter_get_mapping(iter)[1],
					segment_get_start(seg_tmp)[1], segment_get_size(seg_tmp)[1])
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

		(err, seg1) = segment_new("abcdef", 0, 6, None)

		(err, seg2) = segment_new("012345", 0, 6, None)

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
		segcol_iter_free(iter)

		iter = segcol_find(self.segcol, 5)[1]
		seg = segcol_iter_get_segment(iter)[1]

		self.assertEqual(seg, seg1)
		segcol_iter_free(iter)

		# Check seg2 corner cases
		iter = segcol_find(self.segcol, 6)[1]
		seg = segcol_iter_get_segment(iter)[1]

		self.assertEqual(seg, seg2)
		segcol_iter_free(iter)

		iter = segcol_find(self.segcol, 11)[1]
		seg = segcol_iter_get_segment(iter)[1]

		self.assertEqual(seg, seg2)
		segcol_iter_free(iter)
	
	def testIterator(self):
		"Create and traverse an iterator"

		nseg = 10
		seg = []

		for i in xrange(nseg):
			(err, seg_tmp) = segment_new("%d%d" % (i, i), 0, 2, None)
			seg.append(("%d%d" % (i, i), 2 * i, 0, 2))
			segcol_append(self.segcol, seg_tmp)

		self.check_iter_segments(self.segcol, seg)
		
	def testInsertBeginning(self):
		"Insert a segment at the beginning of another segment"

		# Append some segments to the segcol
		self.testAppend()

		(err, seg1) = segment_new("BBB", 0, 3, None)

		# Insert segment at the beginning of anothe segment
		self.assertEqual(segcol_insert(self.segcol, 0, seg1), 0)
		self.assertEqual(segcol_get_size(self.segcol)[1], 15)

		# Segcol should be ["BBB"]-["abcdef"]-["012345"]
		segs = [("BBB", 0, 0, 3), ("abcdef", 3, 0, 6), ("012345", 9, 0, 6)]

		self.check_iter_segments(self.segcol, segs)
		
	def testInsertEnd(self):
		"Insert a segment at the end of another segment"

		# Insert some segments into the segcol
		self.testInsertBeginning()

		(err, seg1) = segment_new("EEE", 0, 3, None)

		# Insert segment at the end of another segment
		segcol_insert(self.segcol, 8, seg1)
		self.assertEqual(segcol_get_size(self.segcol)[1], 18)

		# Segcol should be ["BBB"]-["abcde"f]-["EEE"]-[abcde"f"]-["012345"]
		segs = [("BBB", 0, 0, 3), ("abcdef", 3, 0, 5), ("EEE", 8, 0, 3),
				("abcdef", 11, 5, 1), ("012345", 12, 0, 6)]

		self.check_iter_segments(self.segcol, segs)

	def testInsertMiddle(self):
		"Insert a segment in the middle of another segment"

		# Insert some segments into the segcol
		self.testInsertEnd()

		(err, seg1) = segment_new("MMM", 0, 3, None)

		# Insert segment at the end of another segment
		segcol_insert(self.segcol, 5, seg1)
		self.assertEqual(segcol_get_size(self.segcol)[1], 21)

		# Segcol should be ["BBB"]-["ab"cdef]-["MMM"]-[ab"cde"f]-["EEE"]
		# -[abcde"f"]-["012345"]
		segs = [("BBB", 0, 0, 3), ("abcdef", 3, 0, 2), ("MMM", 5, 0, 3), 
				("abcdef", 8, 2, 3), ("EEE", 11, 0, 3), ("abcdef", 14, 5, 1),
				("012345", 15, 0, 6)]

		self.check_iter_segments(self.segcol, segs)

	def testTryInsertNegative(self):
		"Try to insert a segment at a negative index"

		(err, seg1) = segment_new("ERROR", 0, 5, None)
		
		err = segcol_insert(self.segcol, -1, seg1)
		self.assertNotEqual(err, 0)

		# Insert some segments into the segcol
		self.testInsertBeginning()

		err = segcol_insert(self.segcol, -1, seg1)
		self.assertNotEqual(err, 0)

		# Segcol should remain ["BBB"]-["abcdef"]-["012345"]
		segs = [("BBB", 0, 0, 3), ("abcdef", 3, 0, 6), ("012345", 9, 0, 6)]

		self.check_iter_segments(self.segcol, segs)

	def testTryInsertBeyondEOF(self):
		"Try to insert a segment beyond the end of a buffer"

		(err, seg1) = segment_new("ERROR", 0, 5, None)
		
		err = segcol_insert(self.segcol, 0, seg1)
		self.assertNotEqual(err, 0)

		# Insert some segments into the segcol
		self.testInsertBeginning()
		size = segcol_get_size(self.segcol)[1]

		err = segcol_insert(self.segcol, size, seg1)
		self.assertNotEqual(err, 0)

		# Segcol should remain ["BBB"]-["abcdef"]-["012345"]
		segs = [("BBB", 0, 0, 3), ("abcdef", 3, 0, 6), ("012345", 9, 0, 6)]

		self.check_iter_segments(self.segcol, segs)

		segment_free(seg1)

	def testDeleteBeginning(self):
		"Delete a range from the beginning of a segment"

		# Append some segments to the segcol
		self.testAppend()

		(err, del_segcol) = segcol_delete(self.segcol, 0, 4) 
		self.assertEqual(err, 0)

		# Segcol should now be [abcd"ef"]-["012345"]
		segs = [("abcdef", 0, 4, 2), ("012345", 2, 0, 6)]
		self.check_iter_segments(self.segcol, segs)

		# Deleted segcol should be ["abcd"ef]
		segs = [("abcdef", 0, 0, 4)]
		self.check_iter_segments(del_segcol, segs)

		segcol_free(del_segcol)

		(err, del_segcol) = segcol_delete(self.segcol, 2, 6) 
		self.assertEqual(err, 0)

		# Segcol should now be [abcd"ef"]
		segs = [("abcdef", 0, 4, 2)]
		self.check_iter_segments(self.segcol, segs)

		# Deleted segcol should be ["012345"]
		segs = [("012345", 0, 0, 6)]
		self.check_iter_segments(del_segcol, segs)

		segcol_free(del_segcol)

		(err, del_segcol) = segcol_delete(self.segcol, 0, 2) 
		self.assertEqual(err, 0)

		# Segcol should now be empty
		segs = []
		self.check_iter_segments(self.segcol, segs)

		# Deleted segcol should be [abcd"ef"]
		segs = [("abcdef", 0, 4, 2)]
		self.check_iter_segments(del_segcol, segs)
		
		segcol_free(del_segcol)

	def testDeleteMiddle(self):
		"Delete a range from the middle of a segment"

		# Append a segment to the segcol
		(err, seg1) = segment_new("0123456789", 0, 10, None)

		segcol_append(self.segcol, seg1)
		self.assertEqual(segcol_get_size(self.segcol)[1], 10)

		for i in xrange(4, 0, -1):
			(err, del_segcol) = segcol_delete(self.segcol, i, 2)
			self.assertEqual(err, 0)
			
			segs = [("0123456789", 0, 0, i), ("0123456789", i, 10 - i, i)]
			self.check_iter_segments(self.segcol, segs)

			if i == 4:
				segs = [("0123456789", 0, i, 2)]
			else:
				segs = [("0123456789", 0, i, 1), ("0123456789", 1, 9 - i, 1)]

			self.check_iter_segments(del_segcol, segs)

			segcol_free(del_segcol)

	def testDeleteMiddleMulti(self):
		"Delete a range from the middle of multiple segments"

		# Append some segments to the segcol
		self.testAppend()

		for i in xrange(5, 0, -1):
			(err, del_segcol) = segcol_delete(self.segcol, i, 2)
			self.assertEqual(err, 0)
			
			# Segcol should now be ["abcde"f]-[0"12345"]
			segs = [("abcdef", 0, 0, i), ("012345", i, 6 - i, i)]
			self.check_iter_segments(self.segcol, segs)

			# Deleted segcol should be ["abcde"f]-[0"12345"]
			segs = [("abcdef", 0, i, 1), ("012345", 1, 6 - i - 1, 1)]
			self.check_iter_segments(del_segcol, segs)

			segcol_free(del_segcol)

	def testDeleteNoDeleted(self):
		"Delete a range from a segcol but don't ask for the deleted segments"

		# Append some segments to the segcol
		self.testAppend()

		for i in xrange(5, 0, -1):
			segcol_get_size(self.segcol)
			err = segcol_delete_no_deleted(self.segcol, i, 2)
			self.assertEqual(err, 0)
			
			# Segcol should now be ["abcde"f]-[0"12345"]
			segs = [("abcdef", 0, 0, i), ("012345", i, 6 - i, i)]
			self.check_iter_segments(self.segcol, segs)

	def testDeleteMany(self):
		"Delete a range affecting many (more that 2) segments from the segcol"

		# Append some segments (>2) to the segcol
		self.testInsertMiddle()

		(err, del_segcol) = segcol_delete(self.segcol, 2, 15)
		self.assertEqual(err, 0)

		# Segcol should be ["BB"B]-[01"2345"]
		segs = [("BBB", 0, 0, 2), ("012345", 2, 2, 4)]

		self.check_iter_segments(self.segcol, segs)

		# deleted segcol should be [BB"B"]-["ab"cdef]-["MMM"]-[ab"cde"f]-["EEE"]
		# -[abcde"f"]-["01"2345]
		del_segs = [("BBB", 0, 2, 1), ("abcdef", 1, 0, 2), ("MMM", 3, 0, 3), 
				("abcdef", 6, 2, 3), ("EEE", 9, 0, 3), ("abcdef", 12, 5, 1),
				("012345", 13, 0, 2)]

		self.check_iter_segments(del_segcol, del_segs)

		segcol_free(del_segcol)

	def testDeleteAll(self):
		"Delete a whole segcol (with >2 segments)"

		# Append some segments (>2) to the segcol
		self.testInsertBeginning()

		(err, size) = segcol_get_size(self.segcol)
		(err, del_segcol) = segcol_delete(self.segcol, 0, size)
		self.assertEqual(err, 0)

		# Segcol should be ["BBB"]-["abcdef"]-["012345"]
		segs = [("BBB", 0, 0, 3), ("abcdef", 3, 0, 6), ("012345", 9, 0, 6)]
		self.check_iter_segments(del_segcol, segs)

		segcol_free(del_segcol)

	def testFindStressTest(self):
		"Find a segment stress test"
		
		segs = []

		for i in xrange(1000):
			(err, seg1) = segment_new("abcdef", 0, 6, None)
			segs.append(seg1)

			segcol_append(self.segcol, seg1)
		
		for i in xrange(6000):
			(err, iter) = segcol_find(self.segcol, i)
			seg = segcol_iter_get_segment(iter)[1]
			mapping = segcol_iter_get_mapping(iter)[1]

			self.assertEqual(mapping, (i/6)*6)

			self.assertEqual(seg, segs[i/6])

			segcol_iter_free(iter)

	def testTryFindInvalidOffset(self):
		"Try to search for invalid offsets"

		(err, iter) = segcol_find(self.segcol, 0)
		self.assertNotEqual(err, 0)

		# Insert some segments into the segcol
		self.testInsertBeginning()
		size = segcol_get_size(self.segcol)[1]

		(err, iter) = segcol_find(self.segcol, size)
		self.assertNotEqual(err, 0)

		# Segcol should remain ["BBB"]-["abcdef"]-["012345"]
		segs = [("BBB", 0, 0, 3), ("abcdef", 3, 0, 6), ("012345", 9, 0, 6)]

		self.check_iter_segments(self.segcol, segs)

	def testAppendOverflow(self):
		"Try boundary conditions for overflow in append"

		# Test 1
		(err, seg1) = segment_new(None, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)

		err = segcol_append(self.segcol, seg1)
		self.assertEqual(err, 0)

		(err, seg2) = segment_new(None, 0, 1, None)
		self.assertEqual(err, 0)

		err = segcol_append(self.segcol, seg2)
		self.assertNotEqual(err, 0)

		segment_free(seg2)

		# Clear segcol
		(err, deleted) = segcol_delete(self.segcol, 0, get_max_off_t())
		segcol_free(deleted)

		# Test 2
		(err, seg1) = segment_new(None, 0, 1, None)
		self.assertEqual(err, 0)

		err = segcol_append(self.segcol, seg1)
		self.assertEqual(err, 0)

		(err, seg2) = segment_new(None, 0, get_max_off_t() - 1, None)
		self.assertEqual(err, 0)

		err = segcol_append(self.segcol, seg2)
		self.assertEqual(err, 0)

		(err, seg2) = segment_new(None, 0, 1, None)
		self.assertEqual(err, 0)

		err = segcol_append(self.segcol, seg2)
		self.assertNotEqual(err, 0)

		segment_free(seg2)

	def testInsertOverflow(self):
		"Try boundary conditions for overflow in insert"

		# Test 1
		(err, seg1) = segment_new(None, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)

		err = segcol_append(self.segcol, seg1)
		self.assertEqual(err, 0)

		(err, seg2) = segment_new(None, 0, 1, None)
		self.assertEqual(err, 0)

		err = segcol_insert(self.segcol, 0, seg2)
		self.assertNotEqual(err, 0)

		err = segcol_insert(self.segcol, get_max_off_t(), seg2)
		self.assertNotEqual(err, 0)

		segment_free(seg2)

		# Clear segcol
		(err, deleted) = segcol_delete(self.segcol, 0, get_max_off_t())
		segcol_free(deleted)

		# Test 2
		(err, seg1) = segment_new(None, 0, 1, None)
		self.assertEqual(err, 0)

		err = segcol_append(self.segcol, seg1)
		self.assertEqual(err, 0)

		(err, seg2) = segment_new(None, 0, get_max_off_t() - 1, None)
		self.assertEqual(err, 0)

		err = segcol_insert(self.segcol, 0, seg2)
		self.assertEqual(err, 0)

		(err, seg2) = segment_new(None, 0, 1, None)
		self.assertEqual(err, 0)

		err = segcol_insert(self.segcol, get_max_off_t(), seg2)
		self.assertNotEqual(err, 0)

		err = segcol_insert(self.segcol, 0, seg2)
		self.assertNotEqual(err, 0)

		segment_free(seg2)

	def testDeleteOverflow(self):
		"Try boundary conditions for overflow in delete"

		(err, seg1) = segment_new(None, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)

		err = segcol_append(self.segcol, seg1)
		self.assertEqual(err, 0)

		(err, deleted) = segcol_delete(self.segcol, 1, get_max_off_t())
		self.assertNotEqual(err, 0)

		(err, deleted) = segcol_delete(self.segcol, get_max_off_t(), 2)
		self.assertNotEqual(err, 0)

if __name__ == '__main__':
	unittest.main()
