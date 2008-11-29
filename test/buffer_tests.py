import unittest
import errno
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

	def testAppendBoundaryCases(self):
		"Try boundary cases for appending to buffer"

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

	def testReadBoundaryCases(self):
		"Try boundary cases for reading from buffer"

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

	def doInsertAtPos(self, pos):
		"Insert data to the start of the buffer"

		read_data = create_string_buffer(100)

		# Append some data to the buffer
		self.testAppend()

		# Insert data
		data = "INSERTEDDATA"
		err = bless_buffer_insert(self.buf, pos, data, len(data))
		self.assertEqual(err, 0)

		# Test insert
		appended_data = "0123456789abcdefghij"
		expected_data = "%s%s%s" % (appended_data[:pos], data, appended_data[pos:])

		err = bless_buffer_read(self.buf, 0, read_data, 0, len(expected_data))
		self.assertEqual(err, 0)

		for i in range(len(expected_data)):
			self.assertEqual(read_data[i], expected_data[i])

	def testInsertStart(self):
		"Insert data to the start of the buffer"
		self.doInsertAtPos(0)

	def testInsertMiddle1(self):
		"Insert data in the middle of the buffer"
		self.doInsertAtPos(5)

	def testInsertMiddle2(self):
		"Insert data in the middle of the buffer"
		self.doInsertAtPos(9)

	def testInsertMiddle3(self):
		"Insert data in the middle of the buffer"
		self.doInsertAtPos(10)

	def testInsertMiddle4(self):
		"Insert data in the middle of the buffer"
		self.doInsertAtPos(19)

	def testInsertBoundaryCases(self):
		"Try some boundary cases of buffer insertion"

		data = "INSERTEDDATA"

		# Try to insert in an empty buffer
		err = bless_buffer_insert(self.buf, 0, data, len(data))
		self.assertNotEqual(err, 0)

		self.testAppend()

		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)

		# Try to insert in a negative offset
		err = bless_buffer_insert(self.buf, -1, data, len(data))
		self.assertNotEqual(err, 0)

		err = bless_buffer_insert(self.buf, -10000, data, len(data))
		self.assertNotEqual(err, 0)
		
		# Try to insert out of range
		err = bless_buffer_insert(self.buf, size, data, len(data))
		self.assertNotEqual(err, 0)

		err = bless_buffer_insert(self.buf, size + 1, data, len(data))
		self.assertNotEqual(err, 0)
	
	def testAppendOverflow1(self):
		"Try boundary conditions for overflow in append (size_t)"

		# Try to append a large block of memory (overflows size_t)
		err = bless_buffer_append_ptr(self.buf, 2, get_max_size_t())
		self.assertEqual(err, errno.EOVERFLOW)

		err = bless_buffer_append_ptr(self.buf, 1, get_max_size_t())
		self.assertEqual(err, 0)

	def testAppendOverflow2(self):
		"Try boundary conditions for overflow in append (off_t)"

		# Add a large segment to the buffer
		(err, seg) = segment_new(0, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)
		
		err = bless_buffer_append_segment(self.buf, seg)
		self.assertEqual(err, 0)

		# Try to append additional data (overflows off_t)
		err = bless_buffer_append_ptr(self.buf, bless_malloc(1), 1)
		self.assertEqual(err, errno.EOVERFLOW)

	def testInsertOverflow1(self):
		"Try boundary conditions for overflow in insert (size_t)"

		err = bless_buffer_append_ptr(self.buf, 1, 1)
		self.assertEqual(err, 0)

		err = bless_buffer_insert_ptr(self.buf, 0, 2, get_max_size_t())
		self.assertEqual(err, errno.EOVERFLOW)

		err = bless_buffer_insert_ptr(self.buf, 0, 1, get_max_size_t())
		self.assertEqual(err, 0)

	def testInsertOverflow2(self):
		"Try boundary conditions for overflow in insert (off_t)"

		# Add a large segment to the buffer
		(err, seg) = segment_new(0, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)
		
		err = bless_buffer_append_segment(self.buf, seg)
		self.assertEqual(err, 0)

		# Try to insert additional data (overflows off_t)
		err = bless_buffer_insert_ptr(self.buf, 0, bless_malloc(1), 1)
		self.assertEqual(err, errno.EOVERFLOW)

if __name__ == '__main__':
	unittest.main()
