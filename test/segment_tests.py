import unittest
from libbless import *

class SegmentTests(unittest.TestCase):

	def testNew(self):
		seg = segment_new()
		self.assertNotEqual(seg, None)


if __name__ == '__main__':
	unittest.main()
