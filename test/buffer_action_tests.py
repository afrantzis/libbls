import unittest
import errno
from ctypes import create_string_buffer
from libbls import *

# This test suite contains only basic tests.
# The buffer_action_t ADT is more thoroughly checked indirectly
# in the buffer_tests.py suite.

class BufferActionTests(unittest.TestCase):

	def setUp(self):
		(err, self.buf) = bless_buffer_new();
		self.assertEqual(err, 0)

	def tearDown(self):
		err = bless_buffer_free(self.buf)
		self.assertEqual(err, 0)

	def testDoAppendAction(self):
		"Append data"

		data = "0123456789" 
		(err, data_src) = bless_buffer_source_memory(data, 10, None)
		self.assertEqual(err, 0)

        # Create and perform the action
		(err, action) = buffer_action_append_new(self.buf, data_src, 0, 10);
		self.assertEqual(err, 0)

		err = buffer_action_do(action)
		self.assertEqual(err, 0)

		err = buffer_action_free(action)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

		# Check buffer
		err, size = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 10)

		read_data = create_string_buffer(10)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 10)
		self.assertEqual(err, 0)

		for i in range(len(read_data)):
			self.assertEqual(read_data[i], data[i])

	def testDoInsertAction(self):
		"Insert data"

		self.testDoAppendAction();

		data = "abc" 
		(err, data_src) = bless_buffer_source_memory(data, 3, None)
		self.assertEqual(err, 0)

        # Create and perform the action
		(err, action) = buffer_action_insert_new(self.buf, 4, data_src, 0, 3);
		self.assertEqual(err, 0)

		err = buffer_action_do(action)
		self.assertEqual(err, 0)

		err = buffer_action_free(action)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

		# Check buffer
		err, size = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 13)

		read_data = create_string_buffer(13)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 13)
		self.assertEqual(err, 0)

		expected_data = "0123abc456789"
		for i in range(len(read_data)):
			self.assertEqual(read_data[i], expected_data[i])

	def testDoDeleteAction(self):
		"Delete data"

		self.testDoInsertAction()

        # Create and perform the action
		(err, action) = buffer_action_delete_new(self.buf, 5, 4)
		self.assertEqual(err, 0)

		err = buffer_action_do(action)
		self.assertEqual(err, 0)

		err = buffer_action_free(action)
		self.assertEqual(err, 0)

		# Check buffer
		err, size = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 9)

		read_data = create_string_buffer(9)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 9)
		self.assertEqual(err, 0)

		expected_data = "0123a6789"
		for i in range(len(read_data)):
			self.assertEqual(read_data[i], expected_data[i])

if __name__ == '__main__':
	unittest.main()
