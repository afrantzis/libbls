import unittest
from libbless import *

class SegcolTestsList(unittest.TestCase):

	def setUp(self):
		(err, self.segcol) = segcol_new("list")
		self.assertEqual(err, 0)

	def tearDown(self):
		segcol_free(self.segcol)

	def testNew(self):
		self.assertNotEqual(self.segcol, None)
		self.assertEqual(segcol_get_size(self.segcol)[1], 0)


if __name__ == '__main__':
	unittest.main()
