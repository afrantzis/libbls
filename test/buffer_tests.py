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
		bless_buffer_free(self.buf)

	def testNew(self):
		(err, size) = bless_buffer_get_size(self.buf)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

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
		read_data = create_string_buffer(data_len)
		err = bless_buffer_read(self.buf, 0, read_data, 0, data_len)
		self.assertEqual(err, 0)
		
		for i in range(len(read_data)):
			self.assertEqual(read_data[i], expected_data[i])
		
	def testSaveSelfOverlapHigher(self):
		"""Save a buffer that contains two self overlaps one unmoved and one 
		moved to higher offsets"""

		(fd1, fd1_path) = get_tmp_copy_file_fd("buffer_test_file1.bin", os.O_RDWR)
		
		(err, fd1_src) = bless_buffer_source_file(fd1, None)
		self.assertEqual(err, 0)

		fd2 = get_file_fd("buffer_test_file2.bin")
		
		(err, fd2_src) = bless_buffer_source_file(fd2, None)
		self.assertEqual(err, 0)

		segment_desc = [(fd1_src, 0, 3), (fd2_src, 0, 3), (fd1_src, 3, 7)]

		self.check_save(fd1, segment_desc, "123abc4567890")

		err = bless_buffer_source_unref(fd1_src)
		self.assertEqual(err, 0)

		err = bless_buffer_source_unref(fd2_src)
		self.assertEqual(err, 0)

		# Remove temporary file
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
		os.remove(fd1_path)

	def testBufferOptions(self):
		"Set and get buffer options"

		(err, val) = bless_buffer_get_option(self.buf, BLESS_BUF_SENTINEL)
		self.assertEqual(err, errno.EINVAL)

		for i in xrange(BLESS_BUF_SENTINEL):
			(err, val) = bless_buffer_get_option(self.buf, i) 
			self.assertEqual(err, 0)
			self.assertEqual(val, None)
			err = bless_buffer_set_option(self.buf, i, 'opt%d' % i)
			self.assertEqual(err, 0)

		for i in xrange(BLESS_BUF_SENTINEL):
			(err, val) = bless_buffer_get_option(self.buf, i) 
			self.assertEqual(err, 0)
			self.assertEqual(val, 'opt%d' % i)


if __name__ == '__main__':
	unittest.main()
