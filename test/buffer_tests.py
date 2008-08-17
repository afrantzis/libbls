import unittest
from libbless import *

class BufferTests(unittest.TestCase):

	def testNew(self):
		buf = bless_buffer_new()
		self.assertNotEqual(buf, None)


if __name__ == '__main__':
	unittest.main()
