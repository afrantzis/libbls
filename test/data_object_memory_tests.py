import unittest
import errno
from libbls import *

class DataObjectMemoryTests(unittest.TestCase):

	def setUp(self):
		(err, self.obj) = data_object_memory_new_ptr(bless_malloc(10), 10)

	def tearDown(self):
		data_object_free(self.obj)

	def testNew(self):
		"Create a segment"

	def testWriteWhole(self):
		"Write to a whole data object"

		data = "0123456789" 
		(err, buf) = data_object_get_data(self.obj, 0, 10, DATA_OBJECT_WRITE)
		buf[:] = data
		self.assertEqual(err, 0)

		(err, buf) = data_object_get_data(self.obj, 0, 10, DATA_OBJECT_READ)
		self.assertEqual(err, 0)

		for i in range(len(buf)):
			self.assertEqual(buf[i], data[i])

	def testWriteStart(self):
		"Write to the start of data object"

		self.testWriteWhole()

		whole_data = "abcd456789"

		data = "abcd" 
		(err, buf) = data_object_get_data(self.obj, 0, 4, DATA_OBJECT_WRITE)
		buf[:] = data
		self.assertEqual(err, 0)

		(err, buf) = data_object_get_data(self.obj, 0, 10, DATA_OBJECT_READ)
		self.assertEqual(err, 0)

		for i in range(len(buf)):
			self.assertEqual(buf[i], whole_data[i])

	def testWriteEnd(self):
		"Write to the end of data object"

		self.testWriteWhole()

		whole_data = "012345wxyz"

		data = "wxyz" 
		(err, buf) = data_object_get_data(self.obj, 6, 4, DATA_OBJECT_WRITE)
		buf[:] = data
		self.assertEqual(err, 0)

		(err, buf) = data_object_get_data(self.obj, 0, 10, DATA_OBJECT_READ)
		self.assertEqual(err, 0)

		for i in range(len(buf)):
			self.assertEqual(buf[i], whole_data[i])

	def testWriteMiddle(self):
		"Write to the middle of data object"

		self.testWriteWhole()

		whole_data = "012klmn789"

		data = "klmn" 
		(err, buf) = data_object_get_data(self.obj, 3, 4, DATA_OBJECT_WRITE)
		buf[:] = data
		self.assertEqual(err, 0)

		(err, buf) = data_object_get_data(self.obj, 0, 10, DATA_OBJECT_READ)
		self.assertEqual(err, 0)

		for i in range(len(buf)):
			self.assertEqual(buf[i], whole_data[i])

	def testReadPartial(self):
		"Read from various points in a data object"

		self.testWriteWhole()

		(err, buf) = data_object_get_data(self.obj, 0, 4, DATA_OBJECT_READ)
		self.assertEqual(err, 0)

		data = "0123"

		for i in range(len(buf)):
			self.assertEqual(buf[i], data[i])

		(err, buf) = data_object_get_data(self.obj, 3, 4, DATA_OBJECT_READ)
		self.assertEqual(err, 0)

		data = "3456"

		for i in range(len(buf)):
			self.assertEqual(buf[i], data[i])

		(err, buf) = data_object_get_data(self.obj, 6, 4, DATA_OBJECT_READ)
		self.assertEqual(err, 0)

		data = "6789"

		for i in range(len(buf)):
			self.assertEqual(buf[i], data[i])

	def testGetSize(self):
		"Get the size of data object"

		(err, size) = data_object_get_size(self.obj)
		self.assertEqual(err, 0)

		self.assertEqual(size, 10)

	def testReadInvalid(self):
		"Try to read from invalid ranges in a data object"

		(err, buf) = data_object_get_data(self.obj, -1, 5, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0, "#1")

		# Yes, this one is actually valid!
		(err, buf) = data_object_get_data(self.obj, 0, 0, DATA_OBJECT_READ)
		self.assertEqual(err, 0, "#2")

		(err, buf) = data_object_get_data(self.obj, 0, 11, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0, "#3")

		(err, buf) = data_object_get_data(self.obj, 5, 17, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0, "#4")

		(err, buf) = data_object_get_data(self.obj, 9, 2, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0, "#5")

		(err, buf) = data_object_get_data(self.obj, 10, 0, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0, "#6")

		(err, buf) = data_object_get_data(self.obj, 10, 1, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0, "#7")

		(err, buf) = data_object_get_data(self.obj, 17, 444, DATA_OBJECT_READ)
		self.assertNotEqual(err, 0, "#8")

	def testNewDataOverflow(self):
		"Test boundary cases for new overflow"

		(err, obj) = data_object_memory_new_ptr(0, get_max_size_t())
		self.assertEqual(err, 0)
		
		(err, obj) = data_object_memory_new_ptr(1, get_max_size_t())
		self.assertEqual(err, 0)

		(err, obj) = data_object_memory_new_ptr(2, get_max_size_t())
		self.assertEqual(err, errno.EOVERFLOW)

		(err, obj) = data_object_memory_new_ptr(get_max_size_t(), 0)
		self.assertEqual(err, 0)
		
		(err, obj) = data_object_memory_new_ptr(get_max_size_t(), 1)
		self.assertEqual(err, 0)

		(err, obj) = data_object_memory_new_ptr(get_max_size_t(), 2)
		self.assertEqual(err, errno.EOVERFLOW)

	def testGetDataOverflow(self):
		"Test boundary cases for get_data overflow"

		(err, obj) = data_object_memory_new_ptr(0, get_max_size_t())
		self.assertEqual(err, 0)

		# This one fails because of invalid range, not overflow
		(err, buf) = data_object_get_data(obj, get_max_off_t(), 0, 
				DATA_OBJECT_READ)
		self.assertEqual(err, errno.EINVAL)

		# But this one fails because of overflow (overflow
		# is checked before range)
		(err, buf) = data_object_get_data(obj, get_max_off_t(), 2,
				DATA_OBJECT_READ)
		self.assertEqual(err, errno.EOVERFLOW)
	
	def testCompare(self):
		"Compare two memory data objects"

		(err, obj1) = data_object_memory_new_ptr(0, get_max_size_t())
		self.assertEqual(err, 0)

		(err, obj2) = data_object_memory_new_ptr(0, get_max_size_t())
		self.assertEqual(err, 0)

		(err, result) = data_object_compare(obj1, obj2)
		self.assertEqual(err, 0)
		self.assertEqual(result, 0)

		(err, obj2) = data_object_memory_new_ptr(0, get_max_size_t() - 1)
		self.assertEqual(err, 0)

		(err, result) = data_object_compare(obj1, obj2)
		self.assertEqual(err, 0)
		self.assertEqual(result, 1)

		(err, obj2) = data_object_memory_new_ptr(1, get_max_size_t())
		self.assertEqual(err, 0)

		(err, result) = data_object_compare(obj1, obj2)
		self.assertEqual(err, 0)
		self.assertEqual(result, 1)

if __name__ == '__main__':
	unittest.main()
