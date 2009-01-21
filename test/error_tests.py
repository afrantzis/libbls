import unittest
import errno
import os
from libbless import *

class ErrorTests(unittest.TestCase):

	def setUp(self):
		pass
	
	def tearDown(self):
		pass

	def testPosix(self):
		"Get error descriptions of posix errors"

		for err in errno.errorcode.keys():
			self.assertEqual(bless_strerror(err), os.strerror(err))
	
	def testAppSpecific(self):
		"Get error descriptions of application specific errors"

		self.assertEqual(bless_strerror(-1), "Not implemented")

if __name__ == '__main__':
	unittest.main()
