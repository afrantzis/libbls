import unittest
from libbless import *

class SegmentTests(unittest.TestCase):

	def setUp(self):
		self.seg = segment_new()
	
	def tearDown(self):
		segment_free(self.seg)

	def testNew(self):
		self.assertNotEqual(self.seg, None)
		self.assertEqual(segment_get_size(self.seg), 0)

	def testChange(self):
		segment_change(self.seg, 666, 112358)
		
		self.assertEqual(segment_get_start(self.seg), 666)
		self.assertEqual(segment_get_end(self.seg), 112358)

		self.assertEqual(segment_get_size(self.seg), 112358 - 666 + 1)


if __name__ == '__main__':
	unittest.main()
