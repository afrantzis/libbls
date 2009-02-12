import unittest
import errno
from libbls import *

class OptionsTests(unittest.TestCase):

	def setUp(self):
		(err, self.options) = options_new(5)
		self.assertEqual(err, 0)
	
	def tearDown(self):
		err = options_free(self.options)
		self.assertEqual(err, 0)

	def testNew(self):
		"Test new options"
		for i in xrange(5):
			(err, opt) = options_get_option(self.options, i)
			self.assertEqual(err, 0)
			self.assertEqual(opt, None)

	def testSetGet(self):
		"Set and get options"
		for i in xrange(5):
			err = options_set_option(self.options, i, "num%d" % i)
			self.assertEqual(err, 0)

		for i in xrange(5):
			(err, opt) = options_get_option(self.options, i)
			self.assertEqual(err, 0)
			self.assertEqual(opt, "num%d" % i)
	
	def testInvalid(self):
		"Try to set and get invalid options"
		err = options_set_option(self.options, 5, "val")
		self.assertEqual(err, errno.EINVAL)

		(err, opt) = options_get_option(self.options, 5)
		self.assertEqual(err, errno.EINVAL)

if __name__ == '__main__':
	unittest.main()
