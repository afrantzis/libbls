import unittest
import errno
from ctypes import create_string_buffer
import os
import shutil
import tempfile
from libbls import *

def get_file_fd(name, flags = os.O_RDONLY):
	"""Get the fd of a file.
	   We need this so that the tests can be executed from within
	   the test directory as well as from the source tree root"""
	try:
		fd = os.open("test/%s" % name, flags)
		return fd
	except:
		pass
	
	fd = os.open("%s" % name, flags)
	return fd

def get_tmp_copy_file_fd(name, flags = os.O_RDONLY):
	"""Get the fd of a temporary file which is a copy of the specified
	file."""

	(tmp_fd, tmp_path) = tempfile.mkstemp();
	os.close(tmp_fd)

	success = False
	try:
		shutil.copyfile("test/%s" % name, tmp_path)
		success = True
	except:
		pass
	
	if not success:
		shutil.copyfile(name, tmp_path)

	fd = os.open(tmp_path, flags)

	return (fd, tmp_path)

class BufferTests(unittest.TestCase):

	def setUp(self):
		(err, self.buf) = bless_buffer_new()
		self.assertEqual(err, 0)
		self.assertNotEqual(self.buf, None)

	def tearDown(self):
		err = bless_buffer_free(self.buf)
		self.assertEqual(err, 0)

	def testNew(self):
		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

	def check_buffer(self, buf, expected_data):
		"Check if the buffer contains the expected_data"

		# Read data from buffer and compare it to expected data
		err, buf_size = bless_buffer_get_size(buf)
		read_data = create_string_buffer(buf_size)
		err = bless_buffer_read(buf, 0, read_data, 0, buf_size)
		self.assertEqual(err, 0)

		self.assertEqual(expected_data, read_data.value)

	def check_rev_id(self, buf, expected_rev_id):
		"Check if the buffer contains the expected_data"

		err, rev_id = bless_buffer_get_revision_id(buf)
		self.assertEqual(err, 0)
		self.assertEqual(rev_id, expected_rev_id)

	def check_save_rev_id(self, buf, expected_rev_id):
		"Check if the buffer contains the expected_data"

		err, rev_id = bless_buffer_get_save_revision_id(buf)
		self.assertEqual(err, 0)
		self.assertEqual(rev_id, expected_rev_id)

	def check_undo_redo(self, undo_list):
		"""Checks if the buffer contains the buffer contains the expected
		data after performing undo and redo operations"""
		for (action, expected, rev_id) in undo_list:
			if action == "undo":
				err = bless_buffer_undo(self.buf)
			elif action == "redo":
				err = bless_buffer_redo(self.buf)

			self.assertEqual(err, 0)
			self.check_buffer(self.buf, expected)
			self.check_rev_id(self.buf, rev_id)

	def undo_redo_mix(self, expected_after_redo, expected_after_undo):
		"Redo, then undo, then redo again"

		err = bless_buffer_redo(self.buf)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, expected_after_redo)

		err = bless_buffer_undo(self.buf)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, expected_after_undo)

		err = bless_buffer_redo(self.buf)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, expected_after_redo)

	def testAppend(self):
		"Append data to the buffer"

		# Append data
		data1 = "0123456789" 
		(err, data1_src) = bless_buffer_source_memory(data1, 10, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, data1_src, 0, 10)
		self.assertEqual(err, 0)
		err = bless_buffer_source_unref(data1_src)
		self.assertEqual(err, 0)

		read_data1 = create_string_buffer(10)
		err = bless_buffer_read(self.buf, 0, read_data1, 0, 10)
		self.assertEqual(err, 0)

		for i in range(len(read_data1)):
			self.assertEqual(read_data1[i], data1[i])

		
		# Append more data
		data2 = "abcdefghij"
		(err, data2_src) = bless_buffer_source_memory(data2, 10, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, data2_src, 0, 10)
		self.assertEqual(err, 0)
		err = bless_buffer_source_unref(data2_src)
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
		(err, data_src) = bless_buffer_source_memory("abcdefghij", 10, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, data_src, 0, 0)
		self.assertEqual(err, 0)
		err = bless_buffer_source_unref(data_src)
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
		(err, data_src) = bless_buffer_source_memory(data, len(data), None)
		self.assertEqual(err, 0)

		err = bless_buffer_insert(self.buf, pos, data_src, 0, len(data))
		self.assertEqual(err, 0)
		err = bless_buffer_source_unref(data_src)
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
		(err, data_src) = bless_buffer_source_memory(data, len(data), None)
		self.assertEqual(err, 0)

		# Try to insert in an empty buffer
		err = bless_buffer_insert(self.buf, 0, data_src, 0, len(data))
		self.assertNotEqual(err, 0)

		self.testAppend()

		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)

		# Try to insert in a negative offset
		err = bless_buffer_insert(self.buf, -1, data_src, 0, len(data))
		self.assertNotEqual(err, 0)

		err = bless_buffer_insert(self.buf, -10000, data_src, 0, len(data))
		self.assertNotEqual(err, 0)
		
		# Try to insert out of range
		err = bless_buffer_insert(self.buf, size, data_src, 0, len(data))
		self.assertNotEqual(err, 0)

		err = bless_buffer_insert(self.buf, size + 1, data_src, 0, len(data))
		self.assertNotEqual(err, 0)
		
		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)
		
	def testSourceMemory(self):
		"Try boundary conditions for overflow in _source_memory (size_t)"

		# Try to append a large block of memory (overflows size_t)
		(err, src) = bless_buffer_source_memory_ptr(2, get_max_size_t(), None)
		self.assertEqual(err, errno.EOVERFLOW)

		(err, src) = bless_buffer_source_memory_ptr(1, get_max_size_t(), None)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

	def testAppendOverflow(self):
		"Try boundary conditions for overflow in append (off_t)"

		# Add a large segment to the buffer
		(err, seg) = segment_new(0, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)
		
		err = bless_buffer_append_segment(self.buf, seg)
		self.assertEqual(err, 0)

		# Try to append additional data (overflows off_t)
		(err, data_src) = bless_buffer_source_memory_ptr(bless_malloc(1), 1, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, data_src, 0, 1)
		self.assertEqual(err, errno.EOVERFLOW)

		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)
		
	def testInsertOverflow(self):
		"Try boundary conditions for overflow in insert (off_t)"
  	
		# Add a large segment to the buffer
		(err, seg) = segment_new(0, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)
		
		err = bless_buffer_append_segment(self.buf, seg)
		self.assertEqual(err, 0)
  	
		# Try to insert additional data (overflows off_t)
		(err, data_src) = bless_buffer_source_memory_ptr(bless_malloc(1), 1, None)
		self.assertEqual(err, 0)

		err = bless_buffer_insert(self.buf, 0, data_src, 0, 1)
		self.assertEqual(err, errno.EOVERFLOW)

		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

	def testReadOverflow1(self):
		"Try boundary conditions for overflow in read (size_t)"

		# Add a large segment to the buffer
		(err, seg) = segment_new(0, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)
		
		err = bless_buffer_append_segment(self.buf, seg)
		self.assertEqual(err, 0)
		
		# Check for overflow
		err = bless_buffer_read_ptr(self.buf, 0, 2, 0, get_max_size_t())
		self.assertEqual(err, errno.EOVERFLOW)

		err = bless_buffer_read_ptr(self.buf, 0, 2, get_max_size_t(), 0)
		self.assertEqual(err, errno.EOVERFLOW)

		err = bless_buffer_read_ptr(self.buf, 0, get_max_size_t(), 2, 0)
		self.assertEqual(err, errno.EOVERFLOW)

		err = bless_buffer_read_ptr(self.buf, 0, get_max_size_t(), 0, 2)
		self.assertEqual(err, errno.EOVERFLOW)

	def testReadOverflow2(self):
		"Try boundary conditions for overflow in read (off_t)"

		# Add a large segment to the buffer
		(err, seg) = segment_new(0, 0, get_max_off_t(), None)
		self.assertEqual(err, 0)

		err = bless_buffer_append_segment(self.buf, seg)
		self.assertEqual(err, 0)

		# Check for overflow
		err = bless_buffer_read_ptr(self.buf, get_max_off_t(), 2, 0, 2);
		self.assertEqual(err, errno.EOVERFLOW)

	def testSourceFile(self):
		"Try to open an invalid file"

		# Try to append from an invalid file
		(err, data_src) = bless_buffer_source_file(-1, None)
		self.assertEqual(err, errno.EBADF)

	def testAppendFromFile(self):
		"Append data to the buffer from a file"

		fd = get_file_fd("buffer_test_file1.bin")
		
		(err, data_src) = bless_buffer_source_file(fd, None)
		self.assertEqual(err, 0)

		# Append data
		err = bless_buffer_append(self.buf, data_src, 0, 10)
		self.assertEqual(err, 0)
		
		read_data = create_string_buffer(10)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 10)
		self.assertEqual(err, 0)
		
		expected_data = "1234567890"
		for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])
		
		# Append more data from the same file
		err = bless_buffer_append(self.buf, data_src, 3, 7)
		self.assertEqual(err, 0)
		
		read_data = create_string_buffer(17)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 17)
		self.assertEqual(err, 0)
		
		expected_data = "12345678904567890"
		for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])
		
		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

		# Append more data from another file
		fd = get_file_fd("buffer_test_file2.bin")
		(err, data_src) = bless_buffer_source_file(fd, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, data_src, 2, 5)
		self.assertEqual(err, 0)
		
		read_data = create_string_buffer(22)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 22)
		self.assertEqual(err, 0)
		
		expected_data = "12345678904567890cdefg"
		for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])
	
		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

	def testAppendFromFileBoundaryCases(self):
		"Try boundary cases for appending to buffer from a file"
		
		fd = get_file_fd("buffer_test_file1.bin")
		
		(err, data_src) = bless_buffer_source_file(fd, None)
		self.assertEqual(err, 0)

		# Try to append zero data (should succeed!)
		err = bless_buffer_append(self.buf, data_src, 0, 0)
		self.assertEqual(err, 0)
		
		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)
		
		# Try to append an invalid range from the file
		err = bless_buffer_append(self.buf, data_src, 0, 11)
		self.assertEqual(err, errno.EINVAL)
		
		err = bless_buffer_append(self.buf, data_src, 1, 10)
		self.assertEqual(err, errno.EINVAL)
		
		err = bless_buffer_append(self.buf, data_src, 10, 0)
		self.assertEqual(err, errno.EINVAL)
		
		err = bless_buffer_append(self.buf, data_src, 10, 1)
		self.assertEqual(err, errno.EINVAL)

		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

	def testInsertFromFile(self):
		"Insert data to the buffer from a file"

		fd = get_file_fd("buffer_test_file1.bin")
		
		(err, data_src) = bless_buffer_source_file(fd, None)
		self.assertEqual(err, 0)

		# Append data
		err = bless_buffer_append(self.buf, data_src, 0, 10)
		self.assertEqual(err, 0)
		
		read_data = create_string_buffer(10)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 10)
		self.assertEqual(err, 0)
		
		expected_data = "1234567890"
		for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])
		
		# Insert data from another file 
		fd = get_file_fd("buffer_test_file2.bin")

		(err, data_src1) = bless_buffer_source_file(fd, None)
		self.assertEqual(err, 0)

		err = bless_buffer_insert(self.buf, 2, data_src1, 3, 7)
		self.assertEqual(err, 0)
		
		read_data = create_string_buffer(17)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 17)
		self.assertEqual(err, 0)
		
		expected_data = "12defghij34567890"
		for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])
		
		# Insert more data from the first file
		err = bless_buffer_insert(self.buf, 4, data_src, 2, 5)
		self.assertEqual(err, 0)
		
		read_data = create_string_buffer(22)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 22)
		self.assertEqual(err, 0)
		
		expected_data = "12de34567fghij34567890"
		for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])
	
		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(data_src1)
		self.assertEqual(err, 0)

	def testInsertFromFileBoundaryCases(self):
		"Try boundary cases for inserting to buffer from a file"
		
		fd = get_file_fd("buffer_test_file1.bin")
		
		(err, data_src) = bless_buffer_source_file(fd, None)
		self.assertEqual(err, 0)

		# Append data
		err = bless_buffer_append(self.buf, data_src, 0, 10)
		self.assertEqual(err, 0)
		
		read_data = create_string_buffer(10)
		err = bless_buffer_read(self.buf, 0, read_data, 0, 10)
		self.assertEqual(err, 0)
		
		expected_data = "1234567890"
		for i in range(len(read_data)):
			self.assertEqual(read_data[i], expected_data[i])
		
		# Try to insert zero data (should succeed!)
		err = bless_buffer_insert(self.buf, 0, data_src, 0, 0)
		self.assertEqual(err, 0)
		
		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 10)
		
		# Try to insert an invalid range from the file
		err = bless_buffer_insert(self.buf, 0, data_src, 0, 11)
		self.assertEqual(err, errno.EINVAL)
		
		err = bless_buffer_insert(self.buf, 0, data_src, 1, 10)
		self.assertEqual(err, errno.EINVAL)
		
		err = bless_buffer_insert(self.buf, 0, data_src, 10, 0)
		self.assertEqual(err, errno.EINVAL)
		
		err = bless_buffer_insert(self.buf, 0, data_src, 10, 1)
		self.assertEqual(err, errno.EINVAL)

		err = bless_buffer_source_unref(data_src)
		self.assertEqual(err, 0)

	def testDelete(self):
		"Delete data from the buffer"

		self.testInsertFromFile()

		data = "12de34567fghij34567890"

		for i in range(len(data)/2, 1, -1):
			err = bless_buffer_delete(self.buf, i - 1, 2)
			self.assertEqual(err, 0)

			read_data = create_string_buffer(2 * (i - 1))
			err = bless_buffer_read(self.buf, 0, read_data, 0, len(read_data))
			self.assertEqual(err, 0)

			expected_data = data[:i-1] + data[len(data)-i+1:]

			for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])
			
	def testDeleteBoundaryCases(self):
		"Try boundary cases for deleting from a buffer"

		# Try to delete from an empty buffer
		err = bless_buffer_delete(self.buf, 0, 0)
		self.assertNotEqual(err, 0)
		
		# Insert some data
		self.testInsertFromFile()
		
		(err, buf_size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)

		# Try to delete zero data (should succeed!)
		err = bless_buffer_delete(self.buf, 0, 0)
		self.assertEqual(err, 0)
		
		err = bless_buffer_delete(self.buf, 1, 0)
		self.assertEqual(err, 0)

		# Make sure that buffer didn't change
		(err, buf_size1) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(buf_size1, buf_size)

		# Try to delete from an invalid range
		err = bless_buffer_delete(self.buf, 0, buf_size + 1)
		self.assertNotEqual(err, 0)

		err = bless_buffer_delete(self.buf, -1, buf_size)
		self.assertNotEqual(err, 0)
		
		err = bless_buffer_delete(self.buf, buf_size, 2)
		self.assertNotEqual(err, 0)

		err = bless_buffer_delete(self.buf, buf_size, -1)
		self.assertNotEqual(err, 0)

	def check_save(self, save_fd, segments_desc, expected_data):
		"""Helper function for checking buffer save.
		save_fd: the fd of the file to save to.
		segments_desc: a sequence of tuples (data_obj, offste, length)
		describing the segments to put in the buffer.
		expected_data: a string of data to expect in the buffer after
		having inserted the segments."""

		# Append segments to the buffer
		for (data_obj, offset, length) in segments_desc:
			err = bless_buffer_append(self.buf, data_obj, offset, length)
			self.assertEqual(err, 0)

		# Check file contents before save
		data_len = len(expected_data)
		self.assertEqual(data_len, bless_buffer_get_size(self.buf)[1])
		if data_len > 0:
			read_data = create_string_buffer(data_len)
			err = bless_buffer_read(self.buf, 0, read_data, 0, data_len)
			self.assertEqual(err, 0)

			for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])

		# Save file
		err = bless_buffer_save(self.buf, save_fd, None)
		self.assertEqual(err, 0)

		# Check file contents after save
		self.assertEqual(data_len, bless_buffer_get_size(self.buf)[1])
		if data_len > 0:
			read_data = create_string_buffer(data_len)
			err = bless_buffer_read(self.buf, 0, read_data, 0, data_len)
			self.assertEqual(err, 0)
			
			for i in range(len(read_data)):
				self.assertEqual(read_data[i], expected_data[i])
		
	def testSaveEmpty(self):
		"""Save a empty buffer"""
		(fd1, fd1_path) = get_tmp_copy_file_fd("/dev/null", os.O_RDWR)
		self.check_save(fd1, [], "")
		self.check_save_rev_id(self.buf, 0)
		os.close(fd1)
		os.remove(fd1_path)

	def testSaveSelfOverlapHigher(self):
		"""Save a buffer that contains two self overlaps one unmoved and one 
		moved to higher offsets"""

		# Check all possible higher offset overlaps
		for i in range(7):
			(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin", os.O_RDWR)
			
			(err, fd1_src) = bless_buffer_source_file(fd1, None)
			self.assertEqual(err, 0)

			fd2 = get_file_fd("buffer_test_file2.bin")
			
			(err, fd2_src) = bless_buffer_source_file(fd2, None)
			self.assertEqual(err, 0)

			segment_desc = [(fd1_src, 0, 3), (fd2_src, 0, i), (fd1_src, 3, 7)]
			self.check_save(fd1, segment_desc, "123" + "abcdefgh"[0:i] + "4567890")
			bless_buffer_delete(self.buf, 0, bless_buffer_get_size(self.buf)[1])
			self.check_save_rev_id(self.buf, 4 * i + 3)

			err = bless_buffer_source_unref(fd1_src)
			self.assertEqual(err, 0)

			err = bless_buffer_source_unref(fd2_src)
			self.assertEqual(err, 0)

			# Remove temporary file
			os.close(fd1)
			os.remove(fd1_path)

	def testSaveSelfOverlapLower(self):
		"""Save a buffer that contains two self overlaps one unmoved and one 
		moved to lower offsets"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin", os.O_RDWR)
		
		(err, fd1_src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		fd2 = get_file_fd("buffer_test_file2.bin")
		
		(err, fd2_src) = bless_buffer_source_file(fd2, None)
		self.assertEqual(err, 0)

		segment_desc = [(fd2_src, 0, 1), (fd1_src, 3, 4), (fd2_src, 1, 2), 
				(fd1_src, 7, 3)]

		self.check_save(fd1, segment_desc, "a4567bc890")

		err = bless_buffer_source_unref(fd1_src)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(fd2_src)
		self.assertEqual(err, 0)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testSaveRemoveLeftOverlap(self):
		"""Save a buffer that contains two circular overlaps so that the
		removed overlap is the one where the buffer segment is left of the
		original file segment"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin", os.O_RDWR)
		
		(err, fd1_src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		fd2 = get_file_fd("buffer_test_file2.bin")
		
		(err, fd2_src) = bless_buffer_source_file(fd2, None)
		self.assertEqual(err, 0)

		segment_desc = [(fd1_src, 6, 3), (fd2_src, 0, 4), (fd1_src, 2, 3)]

		self.check_save(fd1, segment_desc, "789abcd345")

		err = bless_buffer_source_unref(fd1_src)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(fd2_src)
		self.assertEqual(err, 0)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testSaveRemoveRightOverlap(self):
		"""Save a buffer that contains two circular overlaps so that the
		removed overlap is the one where the buffer segment is right of the
		original file segment"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin", os.O_RDWR)
		
		(err, fd1_src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		fd2 = get_file_fd("buffer_test_file2.bin")
		
		(err, fd2_src) = bless_buffer_source_file(fd2, None)
		self.assertEqual(err, 0)

		segment_desc = [(fd1_src, 5, 3), (fd2_src, 3, 4), (fd1_src, 1, 3)]

		self.check_save(fd1, segment_desc, "678defg234")

		err = bless_buffer_source_unref(fd1_src)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(fd2_src)
		self.assertEqual(err, 0)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testSaveRemoveBufferContainsFileOverlap(self):
		"""Save a buffer that contains two circular overlaps so that the
		removed overlap is the one where the buffer segment contains the 
		original file segment"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin", os.O_RDWR)
		
		(err, fd1_src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		fd2 = get_file_fd("buffer_test_file2.bin")
		
		(err, fd2_src) = bless_buffer_source_file(fd2, None)
		self.assertEqual(err, 0)

		# Append data
		segment_desc = [ (fd1_src, 5, 5), (fd2_src, 9, 1), (fd1_src, 1, 3),
				(fd2_src, 0, 1)]

		self.check_save(fd1, segment_desc, "67890j234a")

		err = bless_buffer_source_unref(fd1_src)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(fd2_src)
		self.assertEqual(err, 0)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testSaveRemoveFileContainsBufferOverlap(self):
		"""Save a buffer that contains two circular overlaps so that the
		removed overlap is the one where the buffer segment is contained
		in the original file segment"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin", os.O_RDWR)
		
		(err, fd1_src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		fd2 = get_file_fd("buffer_test_file2.bin")
		
		(err, fd2_src) = bless_buffer_source_file(fd2, None)
		self.assertEqual(err, 0)

		# Append data
		segment_desc = [ (fd2_src, 9, 1), (fd1_src, 6, 3), (fd2_src, 0, 1),
				(fd1_src, 0, 4)]

		self.check_save(fd1, segment_desc, "j789a1234")

		err = bless_buffer_source_unref(fd1_src)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(fd2_src)
		self.assertEqual(err, 0)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testSaveDoubleCircularSelfOverlap(self):
		"""Save a buffer that contains two circular overlaps and the segments
		also overlap with themselves"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin", os.O_RDWR)
		
		(err, fd1_src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		fd2 = get_file_fd("buffer_test_file2.bin")
		
		(err, fd2_src) = bless_buffer_source_file(fd2, None)
		self.assertEqual(err, 0)

		segment_desc = [(fd1_src, 1, 3), (fd1_src, 7, 3), (fd1_src, 2, 2),
				(fd1_src, 7, 3)]

		self.check_save(fd1, segment_desc, "23489034890")

		err = bless_buffer_source_unref(fd1_src)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(fd2_src)
		self.assertEqual(err, 0)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testBufferOptions(self):
		"Set and get buffer options"

		(err, val) = bless_buffer_get_option(self.buf, BLESS_BUF_SENTINEL)
		self.assertEqual(err, errno.EINVAL)

		# BLESS_BUF_TMP_DIR
		(err, val) = bless_buffer_get_option(self.buf, BLESS_BUF_TMP_DIR)
		self.assertEqual(err, 0)
		self.assertEqual(val, '/tmp')

		err = bless_buffer_set_option(self.buf, BLESS_BUF_TMP_DIR, '/mydir/tmp')
		self.assertEqual(err, 0)

		(err, val) = bless_buffer_get_option(self.buf, BLESS_BUF_TMP_DIR) 
		self.assertEqual(err, 0)
		self.assertEqual(val, '/mydir/tmp')

		# BLESS_BUF_UNDO_LIMIT
		(err, val) = bless_buffer_get_option(self.buf, BLESS_BUF_UNDO_LIMIT)
		self.assertEqual(err, 0)
		self.assertEqual(val, 'infinite')

		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '2rst')
		self.assertEqual(err, errno.EINVAL)

		(err, val) = bless_buffer_get_option(self.buf, BLESS_BUF_UNDO_LIMIT)
		self.assertEqual(err, 0)
		self.assertEqual(val, 'infinite')

		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '1024')
		self.assertEqual(err, 0)

		(err, val) = bless_buffer_get_option(self.buf, BLESS_BUF_UNDO_LIMIT) 
		self.assertEqual(err, 0)
		self.assertEqual(val, '1024')

	def fill_buffer_for_undo(self):
		data = "0123456789abcdefghij" 
		(err, src) = bless_buffer_source_memory(data, 20, None)
		self.assertEqual(err, 0)

		# Add data
		err = bless_buffer_append(self.buf, src, 0, 10)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "0123456789")

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 1)

		err = bless_buffer_insert(self.buf, 5, src, 10, 3);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "01234abc56789")

		err = bless_buffer_delete(self.buf, 0, 2);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234abc56789")

		err = bless_buffer_insert(self.buf, 0, src, 13, 4);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "defg234abc56789")

		err = bless_buffer_delete(self.buf, 2, 13);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "de")

		err = bless_buffer_append(self.buf, src, 17, 3);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "dehij")

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)


		
	def testBufferUndo(self):
		"Undo buffer actions"

		self.check_rev_id(self.buf, 0)

		# No actions to undo, these should fail
		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)
		
		err = bless_buffer_undo(self.buf)
		self.assertNotEqual(err, 0)

		self.check_rev_id(self.buf, 0)

		self.fill_buffer_for_undo()

		self.check_rev_id(self.buf, 6)

		# Start undoing
		undo_expected = [
				("undo", "de", 5),
				("undo", "defg234abc56789", 4),
				("undo", "234abc56789", 3),
				("undo", "01234abc56789", 2),
				("undo", "0123456789", 1)
				]

		self.check_undo_redo(undo_expected)

		err = bless_buffer_undo(self.buf)
		self.assertEqual(err, 0)

		(err, bufsize) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(bufsize, 0)

		self.check_rev_id(self.buf, 0)

		# No more actions to undo, these should fail
		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

		err = bless_buffer_undo(self.buf)
		self.assertNotEqual(err, 0)

	def testBufferRedo(self):
		"Redo buffer actions"

		self.testBufferUndo()

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

		# Start undo-redo mixes (redo, then undo, then redo again)
		err = bless_buffer_redo(self.buf)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "0123456789")

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 1)

		self.undo_redo_mix("01234abc56789", "0123456789")

		self.undo_redo_mix("234abc56789", "01234abc56789")

		self.undo_redo_mix("defg234abc56789", "234abc56789")

		self.undo_redo_mix("de", "defg234abc56789")

		self.undo_redo_mix("dehij", "de")

		(err, can_redo) = bless_buffer_can_redo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_redo, 0)

		err = bless_buffer_redo(self.buf)
		self.assertNotEqual(err, 0)

	def testBufferUndoMixedEdits(self):
		"Undo buffer actions and perform edit between undos"

		data = "!@#$%^&*()" 
		(err, src) = bless_buffer_source_memory(data, 10, None)
		self.assertEqual(err, 0)

		self.fill_buffer_for_undo()

		undo_expected = [
				("undo", "de", 5),
				("undo", "defg234abc56789", 4),
				]

		self.check_undo_redo(undo_expected)

		(err, can_redo) = bless_buffer_can_redo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_redo, 1)

		err = bless_buffer_insert(self.buf, 5, src, 0, 2)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "defg2!@34abc56789")

		self.check_rev_id(self.buf, 7)

		# We must not be able to redo after performing an action
		(err, can_redo) = bless_buffer_can_redo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_redo, 0)
		
		undo_expected = [
				("undo", "defg234abc56789", 4),
				("redo", "defg2!@34abc56789", 7),
				("undo", "defg234abc56789", 4),
				("undo", "234abc56789", 3),
				]

		self.check_undo_redo(undo_expected)

		err = bless_buffer_delete(self.buf, 0, 6)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "56789")

		self.check_rev_id(self.buf, 8)

		# We must not be able to redo after performing an action
		(err, can_redo) = bless_buffer_can_redo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_redo, 0)

		undo_expected = [
				("undo", "234abc56789", 3),
				("redo", "56789", 8),
				("undo", "234abc56789", 3),
				]

		self.check_undo_redo(undo_expected)

		err = bless_buffer_append(self.buf, src, 3, 6)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234abc56789$%^&*(")

		self.check_rev_id(self.buf, 9)

		# We must not be able to redo after performing an action
		(err, can_redo) = bless_buffer_can_redo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_redo, 0)

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

	def testBufferUndoLimitZero(self):
		"Try to undo actions when the limit is zero"

		# No actions to undo, these should fail
		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)
		
		err = bless_buffer_undo(self.buf)
		self.assertNotEqual(err, 0)

		# Add some data
		data = "0123456789abcdefghij" 
		(err, src) = bless_buffer_source_memory(data, 20, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, src, 0, 10)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "0123456789")

		self.check_rev_id(self.buf, 1)

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

		# Set undo limit
		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '0')
		self.assertEqual(err, 0)

		# The undo limit is 0, we shouldn't be able to undo
		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

		self.check_rev_id(self.buf, 1)

	def testBufferUndoLimit(self):
		"Enforce an undo limit"

		# Set undo limit
		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '2')
		self.assertEqual(err, 0)

		# No actions to undo, these should fail
		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)
		
		err = bless_buffer_undo(self.buf)
		self.assertNotEqual(err, 0)

		# Fill the buffer
		self.fill_buffer_for_undo()

		# Undo some actions
		undo_expected = [
				("undo", "de", 5),
				("undo", "defg234abc56789", 4),
				]

		self.check_undo_redo(undo_expected)
		
		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)


	def testBufferUndoLimitAfter(self):
		"Enforce an undo limit after having performed actions"

		# No actions to undo, these should fail
		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)
		
		err = bless_buffer_undo(self.buf)
		self.assertNotEqual(err, 0)

		# Fill the buffer
		self.fill_buffer_for_undo()

		# Undo some actions
		undo_expected = [
				("undo", "de", 5),
				("undo", "defg234abc56789", 4),
				("undo", "234abc56789", 3),
				]

		self.check_undo_redo(undo_expected)

		# Set undo limit
		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '1')
		self.assertEqual(err, 0)

		(err, can_redo) = bless_buffer_can_redo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_redo, 0)

		undo_expected = [
				("undo", "01234abc56789", 2)
				]

		self.check_undo_redo(undo_expected)

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

	def testBufferUndoLimitIncrease(self):
		"Increase the undo limit"

		# No actions to undo, these should fail
		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)
		
		err = bless_buffer_undo(self.buf)
		self.assertNotEqual(err, 0)

		# Fill the buffer
		self.fill_buffer_for_undo()

		# Undo some actions
		undo_expected = [
				("undo", "de", 5),
				("undo", "defg234abc56789", 4),
				("undo", "234abc56789", 3),
				]

		self.check_undo_redo(undo_expected)

		# Set undo limit
		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '2')
		self.assertEqual(err, 0)

		undo_expected = [
				("undo", "01234abc56789", 2),
				("redo", "234abc56789", 3),
				]
		self.check_undo_redo(undo_expected)

		# Increase undo limit
		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '4')
		self.assertEqual(err, 0)

		# Some more actions
		data = "!@#$%^&*()" 
		(err, src) = bless_buffer_source_memory(data, 10, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, src, 0, 2)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234abc56789!@")

		err = bless_buffer_insert(self.buf, 5, src, 2, 2)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234ab#$c56789!@")

		err = bless_buffer_delete(self.buf, 8, 3)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234ab#$c89!@")

		err = bless_buffer_append(self.buf, src, 8, 2)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234ab#$c89!@()")

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

		# Only the last 4 actions should be available
		undo_expected = [
				("undo", "234ab#$c89!@", 9),
				("undo", "234ab#$c56789!@", 8),
				("undo", "234abc56789!@", 7),
				("undo", "234abc56789", 3)
				]

		self.check_undo_redo(undo_expected)

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

	def testUndoAfterSave(self):
		"Undo actions after having saved a file"

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin",
				os.O_RDWR)
		
		(err, src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		# Perform actions
		err = bless_buffer_append(self.buf, src, 5, 5)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "67890")

		err = bless_buffer_append(self.buf, src, 0, 5)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "6789012345")

		err = bless_buffer_delete(self.buf, 3, 4)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "678345")

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

		# Save
		err = bless_buffer_save(self.buf, fd1, None)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "678345")
		self.check_save_rev_id(self.buf, 3)

		# Undo 
		undo_expected = [
				("undo", "6789012345", 2),
				("undo", "67890", 1)
				]

		self.check_undo_redo(undo_expected)

		err = bless_buffer_undo(self.buf)
		self.assertEqual(err, 0)

		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

		# Redo
		redo_expected = [
				("redo", "67890", 1),
				("redo", "6789012345", 2),
				("redo", "678345", 3),
				]

		self.check_undo_redo(redo_expected)
		self.check_save_rev_id(self.buf, 3)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testUndoAfterSaveNeverOption(self):
		"""Try to undo actions after having saved a buffer that has
		the BLESS_BUF_UNDO_AFTER_SAVE option set to 'never'"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin",
				os.O_RDWR)
		
		(err, src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		# Perform actions
		err = bless_buffer_append(self.buf, src, 5, 5)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "67890")

		err = bless_buffer_append(self.buf, src, 0, 5)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "6789012345")

		err = bless_buffer_delete(self.buf, 3, 4)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "678345")

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

		self.check_rev_id(self.buf, 3)

		# Save
		err = bless_buffer_set_option(self.buf,
				BLESS_BUF_UNDO_AFTER_SAVE, "never");
		self.assertEqual(err, 0)

		# Save
		err = bless_buffer_save(self.buf, fd1, None)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "678345")
		self.check_save_rev_id(self.buf, 3)

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

		(err, can_redo) = bless_buffer_can_redo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_redo, 0)

		self.check_rev_id(self.buf, 3)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testUndoAfterSaveNeverOptionFail(self):
		"""Undo actions after having saved a buffer that has
		the BLESS_BUF_UNDO_AFTER_SAVE option set to 'never' but the
		save failed"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin",
				os.O_RDONLY)
		
		(err, src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		# Perform actions
		err = bless_buffer_append(self.buf, src, 5, 5)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "67890")

		err = bless_buffer_append(self.buf, src, 0, 5)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "6789012345")

		err = bless_buffer_delete(self.buf, 3, 4)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "678345")

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

		# Save
		err = bless_buffer_set_option(self.buf,
				BLESS_BUF_UNDO_AFTER_SAVE, "never");
		self.assertEqual(err, 0)

		# Save (should fail because fd1 is read-only)
		err = bless_buffer_save(self.buf, fd1, None)
		self.assertNotEqual(err, 0)
		self.check_buffer(self.buf, "678345")
		self.check_rev_id(self.buf, 3)
		self.check_save_rev_id(self.buf, 0)

		# We should be able to Undo/Redo normally regardless of the
		# BLESS_BUF_UNDO_AFTER_SAVE = "never" option because the save failed
		undo_expected = [
				("undo", "6789012345", 2),
				("undo", "67890", 1)
				]

		self.check_undo_redo(undo_expected)

		err = bless_buffer_undo(self.buf)
		self.assertEqual(err, 0)

		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

		# Redo
		redo_expected = [
				("redo", "67890", 1),
				("redo", "6789012345", 2),
				("redo", "678345", 3),
				]

		self.check_undo_redo(redo_expected)

		# Remove temporary file
		os.close(fd1)
		os.remove(fd1_path)

	def testMultiAction(self):
		"Group multiple actions in one and try to undo and redo the group"

		data = "0123456789abcdefghij" 
		(err, src) = bless_buffer_source_memory(data, 20, None)
		self.assertEqual(err, 0)

		# Add data
		err = bless_buffer_append(self.buf, src, 0, 10)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "0123456789")

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 1)

		err = bless_buffer_insert(self.buf, 5, src, 10, 3);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "01234abc56789")

		# Begin a multi action
		err = bless_buffer_begin_multi_action(self.buf)
		self.assertEqual(err, 0)

		err = bless_buffer_delete(self.buf, 0, 2);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234abc56789")

		err = bless_buffer_insert(self.buf, 0, src, 13, 4);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "defg234abc56789")

		err = bless_buffer_delete(self.buf, 2, 13);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "de")

		# End the multi action
		err = bless_buffer_end_multi_action(self.buf)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, src, 0, 3);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "de012")

		undo_expected = [
				("undo", "de", 3),
				("undo", "01234abc56789", 2),
				("undo", "0123456789", 1),
				]

		self.check_undo_redo(undo_expected)

		err = bless_buffer_undo(self.buf)
		self.assertEqual(err, 0)

		(err, buf_size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(buf_size, 0)

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

		# Redo the actions
		undo_expected = [
				("redo", "0123456789", 1),
				("redo", "01234abc56789", 2),
				("redo", "de", 3),
				("redo", "de012", 4),
				]

		self.check_undo_redo(undo_expected)

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

	def testMultiActionUndoLimitOne(self):
		"Change the undo limit to 1 while in multi-action mode"

		data = "0123456789abcdefghij" 
		(err, src) = bless_buffer_source_memory(data, 20, None)
		self.assertEqual(err, 0)

		# Add data
		err = bless_buffer_append(self.buf, src, 0, 10)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "0123456789")

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 1)

		err = bless_buffer_insert(self.buf, 5, src, 10, 3);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "01234abc56789")

		# Begin a multi action
		err = bless_buffer_begin_multi_action(self.buf)
		self.assertEqual(err, 0)

		err = bless_buffer_delete(self.buf, 0, 2);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234abc56789")

		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '1')
		self.assertEqual(err, 0)

		err = bless_buffer_insert(self.buf, 0, src, 13, 4);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "defg234abc56789")

		err = bless_buffer_delete(self.buf, 2, 13);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "de")

		# End the multi action
		err = bless_buffer_end_multi_action(self.buf)
		self.assertEqual(err, 0)

		undo_expected = [
				("undo", "01234abc56789", 2),
				("redo", "de", 3),
				("undo", "01234abc56789", 2),
				]

		self.check_undo_redo(undo_expected)

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

		err = bless_buffer_undo(self.buf)
		self.assertNotEqual(err, 0)

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

	def testMultiActionUndoLimitZero(self):
		"Change the undo limit to zero while in multi-action mode"

		data = "0123456789abcdefghij" 
		(err, src) = bless_buffer_source_memory(data, 20, None)
		self.assertEqual(err, 0)

		# Add data
		err = bless_buffer_append(self.buf, src, 0, 10)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "0123456789")

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 1)

		err = bless_buffer_insert(self.buf, 5, src, 10, 3);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "01234abc56789")

		# Begin a multi action
		err = bless_buffer_begin_multi_action(self.buf)
		self.assertEqual(err, 0)

		err = bless_buffer_delete(self.buf, 0, 2);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234abc56789")

		err = bless_buffer_set_option(self.buf, BLESS_BUF_UNDO_LIMIT, '0')
		self.assertEqual(err, 0)

		err = bless_buffer_insert(self.buf, 0, src, 13, 4);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "defg234abc56789")

		err = bless_buffer_delete(self.buf, 2, 13);
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "de")

		# End the multi action
		err = bless_buffer_end_multi_action(self.buf)
		self.assertEqual(err, 0)

		(err, can_undo) = bless_buffer_can_undo(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(can_undo, 0)

		err = bless_buffer_undo(self.buf)
		self.assertNotEqual(err, 0)

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

	def testMultiActionCallMany(self):
		"Call the multi action functions multiple times"

		data = "0123456789abcdefghij"
		(err, src) = bless_buffer_source_memory(data, 20, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, src, 10, 3)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "abc")

		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 0)

		# Try to end an inexistent multi action
		err = bless_buffer_end_multi_action(self.buf)
		self.assertNotEqual(err, 0)

		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 0)

		# Call the multi action function 3 times
		err = bless_buffer_begin_multi_action(self.buf)
		self.assertEqual(err, 0)

		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 1)

		err = bless_buffer_append(self.buf, src, 0, 10)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "abc0123456789")

		err = bless_buffer_begin_multi_action(self.buf)
		self.assertEqual(err, 0)

		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 2)

		err = bless_buffer_append(self.buf, src, 0, 10)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "abc01234567890123456789")

		err = bless_buffer_begin_multi_action(self.buf)
		self.assertEqual(err, 0)

		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 3)

		err = bless_buffer_delete(self.buf, 0, 5)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "234567890123456789")

		err = bless_buffer_end_multi_action(self.buf)
		self.assertEqual(err, 0)

		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 2)

		err = bless_buffer_delete(self.buf, 3, 2)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "2347890123456789")

		err = bless_buffer_end_multi_action(self.buf)
		self.assertEqual(err, 0)
		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 1)

		err = bless_buffer_end_multi_action(self.buf)
		self.assertEqual(err, 0)
		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 0)

		err = bless_buffer_end_multi_action(self.buf)
		self.assertNotEqual(err, 0)

		(err, multi) = bless_buffer_query_multi_action(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(multi, 0)

		err = bless_buffer_undo(self.buf)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "abc")

		err = bless_buffer_redo(self.buf)
		self.assertEqual(err, 0)
		self.check_buffer(self.buf, "2347890123456789")

		err = bless_buffer_source_unref(src)
		self.assertEqual(err, 0)

if __name__ == '__main__':
	unittest.main()
