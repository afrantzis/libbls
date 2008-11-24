import unittest
from ctypes import create_string_buffer
from libbless import *

class BufferTests(unittest.TestCase):

	def setUp(self):
		(err, self.buf) = bless_buffer_new()
		self.assertEqual(err, 0)
		self.assertNotEqual(self.buf, None)

	def tearDown(self):
		pass

	def testNew(self):
		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

	def testAppend(self):
		"Append data to the buffer"

		# Append data
		data1 = "0123456789"
		err = bless_buffer_append(self.buf, data1, 10)
		self.assertEqual(err, 0)

		read_data1 = create_string_buffer(10)
		err = bless_buffer_read(self.buf, 0, read_data1, 0, 10)
		self.assertEqual(err, 0)
		
		for i in range(len(read_data1)):
			self.assertEqual(read_data1[i], data1[i])

		# Append more data
		data2 = "abcdefghij"
		err = bless_buffer_append(self.buf, data2, 10)
		self.assertEqual(err, 0)

		read_data2 = create_string_buffer(20)
		err = bless_buffer_read(self.buf, 0, read_data2, 0, 20)
		self.assertEqual(err, 0)
		
		for i in range(len(read_data2)):
			if i >= 10:
				self.assertEqual(read_data2[i], data2[i - 10])
			else:
				self.assertEqual(read_data2[i], data1[i])

	def testAppendCornerCases(self):
		"Try corner cases for appending to buffer"

		# Try to append zero data (should succeed!)
		data = "abcdefghij"
		err = bless_buffer_append(self.buf, data, 0)
		self.assertEqual(err, 0)

		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

	def testRead(self):
		"Read data from the buffer"

		self.testAppend()
		data = "0123456789abcdefghij"

		read_data = create_string_buffer(3)
	
		# Read from various points in the buffer
		for n in range(0, 18):
			err = bless_buffer_read(self.buf, n, read_data, 0, 3)
			self.assertEqual(err, 0)
   	 	
			for i in range(len(read_data)):
				self.assertEqual(read_data[i], data[i + n])

		read_data = create_string_buffer(5)

		for n in range(0, 18):
			err = bless_buffer_read(self.buf, n, read_data, 1, 3)
			self.assertEqual(err, 0)
   	 	
			for i in range(3):
				self.assertEqual(read_data[i + 1], data[i + n])
		
	def testReadCornerCases(self):
		"Try corner cases for reading from buffer"

		read_data = create_string_buffer(100)

		# Try to read data from an empty buffer
		err = bless_buffer_read(self.buf, 0, read_data, 0, 0)
		self.assertNotEqual(err, 0)

		err = bless_buffer_read(self.buf, 4, read_data, 0, 3)
		self.assertNotEqual(err, 0)

		# Append some data to the buffer
		self.testAppend()

		# Try to read zero data (should succeed!)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 0)
		self.assertEqual(err, 0)

		# Try to read from a negative offset
		err = bless_buffer_read(self.buf, -1, read_data, 0, 0)
		self.assertNotEqual(err, 0)

		err = bless_buffer_read(self.buf, -1, read_data, 0, 3)
		self.assertNotEqual(err, 0)

		# Try to read from beyond the end of buffer
		err = bless_buffer_read(self.buf, 0, read_data, 0, 21)
		self.assertNotEqual(err, 0)
	
		err = bless_buffer_read(self.buf, 19, read_data, 0, 2)
		self.assertNotEqual(err, 0)

		err = bless_buffer_read(self.buf, 20, read_data, 0, 0)
		self.assertNotEqual(err, 0)

if __name__ == '__main__':
	unittest.main()
