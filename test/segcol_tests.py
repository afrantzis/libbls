import unittest
from libbless import *

class SegcolTests(unittest.TestCase):

	def testNew(self):
		segcol = segcol_new("list")
		self.assertNotEqual(segcol, None)


if __name__ == '__main__':
	unittest.main()
