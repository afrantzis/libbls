import unittest
import os
import hashlib
import errno
from libbls import *

def get_file_fd(name):
	"""Get the fd of a file.
	   We need this so that the tests can be executed from within
	   the test directory as well as from the source tree root"""
	try:
		fd = os.open("test/%s" % name, os.O_RDONLY)
		return fd
	except:
		pass
	
	fd = os.open("%s" % name, os.O_RDONLY)
	return fd
	
class DataObjectFileTests(unittest.TestCase):

	def setUp(self):
		fd = get_file_fd("data_object_test_file.bin")

		(err, self.obj) = data_object_file_new(fd)

	def tearDown(self):
		if self.obj is not None:
			data_object_free(self.obj)

	def testNew(self):
		"Create a file data object"

		self.assert_(self.obj is not None)

		self.assertEqual(data_object_get_size(self.obj)[1], 10)

	def testGetData(self):
		"Get data from a file data object"

		(err, buf) = data_object_get_data(self.obj, 0, 10, DATA_OBJECT_READ)
		self.assertEqual(err, 0)
		self.assertEqual(len(buf), 10)

		expected_data = "1234567890"

		for i in range(len(buf)):
			self.assertEqual(buf[i], expected_data[i])
		
		(err, buf) = data_object_get_data(self.obj, 5, 5, DATA_OBJECT_READ)
		self.assertEqual(err, 0)
		self.assertEqual(len(buf), 5)

		for i in range(len(buf)):
			self.assertEqual(buf[i], expected_data[5 + i])

		(err, buf) = data_object_get_data(self.obj, 0, 0, DATA_OBJECT_READ)
		self.assertEqual(err, 0)
		self.assertEqual(len(buf), 0)
	
	def testGetDataInvalid(self):
		"Try to get data from invalid offsets"

		(err, buf) = data_object_get_data(self.obj, -1, 10, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0)

		(err, buf) = data_object_get_data(self.obj, 5, 10, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0)

		(err, buf) = data_object_get_data(self.obj, 10, 0, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0)

	def testGetDataLarge(self):
		"Get data from a large file data object"

		# Create the file data object
		try:
			fd = get_file_fd("data_object_test_file_large.bin")
		except:
			print "Warning: Not running data_object_file_tests.testGetDataLarge "\
				"because the file data_object_test_file_large.bin was not found."
			return

		# Get the expected digest
		try:
			f = os.fdopen(get_file_fd("data_object_test_file_large.md5"))
			expected_digest = f.readline().strip()
		except:
			print "Warning: Not running data_object_file_tests.testGetDataLarge "\
				"because the file data_object_test_file_large.md5 was not found."
			return


		(err, dobj) = data_object_file_new(fd)
		self.assertEqual(err, 0)

		(err, size) = data_object_get_size(dobj)
		self.assertEqual(err, 0)

		hash = hashlib.md5()
		bytes_left = size

		# Read the file and update the digest
		while bytes_left > 0:
			(err, buf) = data_object_get_data(dobj, size - bytes_left, 
					bytes_left, DATA_OBJECT_READ)
			
			self.assertEqual(err, 0)

			hash.update(buf)

			bytes_left = bytes_left - len(buf)
			
		digest = hash.hexdigest()

		# Compare our hash digest with the precomputed one
		self.assertEqual(digest, expected_digest)

		data_object_free(dobj)

	def testGetDataOverflow(self):
		"Test boundary cases for get_data overflow"

		# This one fails because of invalid range, not overflow
		(err, buf) = data_object_get_data(self.obj, get_max_off_t(), 0, 
				DATA_OBJECT_READ)
		self.assertEqual(err, errno.EINVAL)

		# But this one fails because of overflow (overflow
		# is checked before range)
		(err, buf) = data_object_get_data(self.obj, get_max_off_t(), 2,
				DATA_OBJECT_READ)
		self.assertEqual(err, errno.EOVERFLOW)

	def testCompare(self):
		"Compare two data objects"

		fd = get_file_fd("data_object_test_file.bin")
		(err, obj1) = data_object_file_new(fd)
		self.assertEqual(err, 0)

		(err, result) = data_object_compare(self.obj, obj1)
		self.assertEqual(err, 0)
		self.assertEqual(result, 0)

		data_object_free(obj1)

		fd = get_file_fd("data_object_file_tests.py")
		(err, obj1) = data_object_file_new(fd)
		self.assertEqual(err, 0)

		(err, result) = data_object_compare(self.obj, obj1)
		self.assertEqual(err, 0)
		self.assertEqual(result, 1)

		# Compare a file object with a memory object

		(err, obj2) = data_object_memory_new_ptr(0, get_max_size_t() - 1)
		self.assertEqual(err, 0)

		(err, result) = data_object_compare(obj1, obj2)
		self.assertEqual(err, 0)
		self.assertEqual(result, 1)

		data_object_free(obj1)
		data_object_free(obj2)

if __name__ == '__main__':
	unittest.main()
