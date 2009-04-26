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
		self.actions = []
		self.assertEqual(err, 0)

	def tearDown(self):
		for action in self.actions:
			err = buffer_action_free(action)
			self.assertEqual(err, 0)

		err = bless_buffer_free(self.buf)
		self.assertEqual(err, 0)
	
	def check_buffer(self, expected_data):
		expected_length = len(expected_data)

		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, expected_length)

		read_data = create_string_buffer(expected_length)
		err = bless_buffer_read(self.buf, 0, read_data, 0, expected_length)
		self.assertEqual(err, 0)

		for i in range(len(read_data)):
			self.assertEqual(read_data[i], expected_data[i])

	def testDoAppendAction(self):
		"Do Append data"

		data = "0123456789" 
		(err, data_src) = bless_buffer_source_memory(data, 10, None)
		self.assertEqual(err, 0)

        # Create and perform the action
		(err, action) = buffer_action_append_new(self.buf, data_src, 0, 10);
		self.assertEqual(err, 0)

		err = buffer_action_do(action)
		self.assertEqual(err, 0)

		self.actions.append(action)

		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

		# Check buffer
		self.check_buffer("0123456789")

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

		self.actions.append(action)

		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

		# Check buffer
		self.check_buffer("0123abc456789")

	def testDoDeleteAction(self):
		"Delete data"

		self.testDoInsertAction()

        # Create and perform the action
		(err, action) = buffer_action_delete_new(self.buf, 5, 4)
		self.assertEqual(err, 0)

		err = buffer_action_do(action)
		self.assertEqual(err, 0)

		self.actions.append(action)

		# Check buffer
		self.check_buffer("0123a6789")

	def testDoDeleteEndAction(self):
		"Delete data at the end of the buffer"

		self.testDoDeleteAction()

        # Create and perform the action
		(err, action) = buffer_action_delete_new(self.buf, 4, 5)
		self.assertEqual(err, 0)

		err = buffer_action_do(action)
		self.assertEqual(err, 0)

		self.actions.append(action)

		# Check buffer
		self.check_buffer("0123")

	def testUndoAppendAction(self):
		"Undo Append data"

		self.testDoAppendAction();

		err = buffer_action_undo(self.actions[0])
		self.assertEqual(err, 0)

		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

	def testUndoInsertAction(self):
		"Undo Append data"

		self.testDoInsertAction();

		err = buffer_action_undo(self.actions[1])
		self.assertEqual(err, 0)

		# Check buffer
		self.check_buffer("0123456789")

		err = buffer_action_undo(self.actions[0])
		self.assertEqual(err, 0)

		# Check buffer
		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

	def testUndoDeleteAction(self):
		"Undo delete data"

		self.testDoDeleteAction();

		err = buffer_action_undo(self.actions[2])
		self.assertEqual(err, 0)

		# Check buffer
		self.check_buffer("0123abc456789")

		err = buffer_action_undo(self.actions[1])
		self.assertEqual(err, 0)

		# Check buffer
		self.check_buffer("0123456789")

		err = buffer_action_undo(self.actions[0])
		self.assertEqual(err, 0)

		# Check buffer
		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

	def testUndoDeleteEndAction(self):
		"Undo delete end data"

		self.testDoDeleteEndAction()

		err = buffer_action_undo(self.actions[3])
		self.assertEqual(err, 0)

		# Check buffer
		self.check_buffer("0123a6789")

		err = buffer_action_undo(self.actions[2])
		self.assertEqual(err, 0)

		# Check buffer
		self.check_buffer("0123abc456789")

		err = buffer_action_undo(self.actions[1])
		self.assertEqual(err, 0)

		# Check buffer
		self.check_buffer("0123456789")

		err = buffer_action_undo(self.actions[0])
		self.assertEqual(err, 0)

		# Check buffer
		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

if __name__ == '__main__':
	unittest.main()
