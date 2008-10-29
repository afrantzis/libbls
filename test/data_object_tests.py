import unittest
from libbless import *

class DataObjectMemoryTests(unittest.TestCase):

	def setUp(self):
		(err, self.obj) = data_object_memory_new(10)
	
	def tearDown(self):
		data_object_free(self.obj)

	def testNew(self):
		"Create a segment"
	
	def testWriteWhole(self):
		"Write to a whole data object"

		data = "0123456789" 
		err = data_object_write(self.obj, 0, data, 10)
		self.assertEqual(err, 0)

		(err, buf) = data_object_read(self.obj, 0, 10)
		self.assertEqual(err, 0)

		for i in range(len(buf)):
			self.assertEqual(buf[i], data[i])

	def testWriteStart(self):
		"Write to the start of data object"

		self.testWriteWhole()

		whole_data = "abcd456789"

		data = "abcd" 
		err = data_object_write(self.obj, 0, data, 4)
		self.assertEqual(err, 0)

		(err, buf) = data_object_read(self.obj, 0, 10)
		self.assertEqual(err, 0)

		for i in range(len(buf)):
			self.assertEqual(buf[i], whole_data[i])

	def testWriteEnd(self):
		"Write to the end of data object"

		self.testWriteWhole()

		whole_data = "012345wxyz"

		data = "wxyz" 
		err = data_object_write(self.obj, 6, data, 4)
		self.assertEqual(err, 0)

		(err, buf) = data_object_read(self.obj, 0, 10)
		self.assertEqual(err, 0)

		for i in range(len(buf)):
			self.assertEqual(buf[i], whole_data[i])

	def testWriteMiddle(self):
		"Write to the middle of data object"

		self.testWriteWhole()

		whole_data = "012klmn789"

		data = "klmn" 
		err = data_object_write(self.obj, 3, data, 4)
		self.assertEqual(err, 0)

		(err, buf) = data_object_read(self.obj, 0, 10)
		self.assertEqual(err, 0)

		for i in range(len(buf)):
			self.assertEqual(buf[i], whole_data[i])

	def testReadPartial(self):
		"Read from various points in a data object"

		self.testWriteWhole()

		(err, buf) = data_object_read(self.obj, 0, 4)
		self.assertEqual(err, 0)

		data = "0123"

		for i in range(len(buf)):
			self.assertEqual(buf[i], data[i])

		(err, buf) = data_object_read(self.obj, 3, 4)
		self.assertEqual(err, 0)

		data = "3456"

		for i in range(len(buf)):
			self.assertEqual(buf[i], data[i])

		(err, buf) = data_object_read(self.obj, 6, 4)
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

		(err, buf) = data_object_read(self.obj, -1, 5)
		self.assertNotEqual(err, 0, "#1")

		# Yes, this one is actually valid!
		(err, buf) = data_object_read(self.obj, 0, 0)
		self.assertEqual(err, 0, "#2")

		(err, buf) = data_object_read(self.obj, 0, 11)
		self.assertNotEqual(err, 0, "#3")
		
		(err, buf) = data_object_read(self.obj, 5, 17)
		self.assertNotEqual(err, 0, "#4")

		(err, buf) = data_object_read(self.obj, 9, 2)
		self.assertNotEqual(err, 0, "#5")

		(err, buf) = data_object_read(self.obj, 10, 0)
		self.assertNotEqual(err, 0, "#6")

		(err, buf) = data_object_read(self.obj, 10, 1)
		self.assertNotEqual(err, 0, "#7")

		(err, buf) = data_object_read(self.obj, 17, 444)
		self.assertNotEqual(err, 0, "#8")

if __name__ == '__main__':
	unittest.main()
