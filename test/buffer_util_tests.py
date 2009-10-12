import unittest
from ctypes import create_string_buffer
from ctypes import c_char_p
import os
import tempfile
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

class BufferUtilTests(unittest.TestCase):

	def setUp(self):
		pass

	def tearDown(self):
		pass


	def testReadDataObject(self):
		"Read from a large (> block size) data object"

		fd = get_file_fd("buffer_tests.py")

		# Read data using python functions
		f = os.fdopen(fd)

		from_python = f.read()

		# Read data using read_data_object
		(err, data_obj) = data_object_file_new(fd)
		self.assertEqual(err, 0)

		read_data = create_string_buffer(len(from_python))
		err = read_data_object(data_obj, 0, read_data, len(from_python))
		self.assertEqual(err, 0)

		self.assertEqual(read_data.value, from_python)

		data_object_free(data_obj)

	def testWriteDataObject(self):
		"Write to a file from a large (> block size) data object"

		(tmp_fd, tmp_path) = tempfile.mkstemp();

		fd = get_file_fd("buffer_tests.py")

		# Read data using python functions
		f = os.fdopen(fd)

		from_python = f.read()

		# Write data using write_data_object
		(err, data_obj) = data_object_file_new(fd)
		self.assertEqual(err, 0)

		err = write_data_object(data_obj, 2, len(from_python) - 4, tmp_fd, 0)
		self.assertEqual(err, 0)

		os.close(tmp_fd)

		tmp_f = open(tmp_path)
		self.assert_(tmp_f is not None)
		from_python_tmp = tmp_f.read()

		self.assertEqual(from_python_tmp, from_python[2:-2])

		os.remove(tmp_path)
		data_object_free(data_obj)
	
	def testWriteDataObjectSafeSmall(self):
		"Write in a safe way to a file from a small (<= block size) data object"

		(tmp_fd, tmp_path) = tempfile.mkstemp();

		fd = get_file_fd("buffer_test_file1.bin")

		# Read data using python functions
		f = os.fdopen(fd)

		from_python = f.read()

		# Write data using write_data_object
		(err, data_obj) = data_object_file_new(fd)
		self.assertEqual(err, 0)

		err = write_data_object_safe(data_obj, 1, len(from_python) - 2, tmp_fd, 0)
		self.assertEqual(err, 0)

		os.close(tmp_fd)

		tmp_f = open(tmp_path)
		self.assert_(tmp_f is not None)
		from_python_tmp = tmp_f.read()

		self.assertEqual(from_python_tmp, from_python[1:-1])

		os.remove(tmp_path)
		data_object_free(data_obj)

	def testWriteDataObjectSafeLarge(self):
		"Write in a safe way to a file from a large (> block size) data object"

		(tmp_fd, tmp_path) = tempfile.mkstemp();

		fd = get_file_fd("buffer_tests.py")

		# Read data using python functions
		f = os.fdopen(fd)

		from_python = f.read()

		# Write data using write_data_object
		(err, data_obj) = data_object_file_new(fd)
		self.assertEqual(err, 0)

		err = write_data_object_safe(data_obj, 2, len(from_python) - 4, tmp_fd, 0)
		self.assertEqual(err, 0)

		os.close(tmp_fd)

		tmp_f = open(tmp_path)
		self.assert_(tmp_f is not None)
		from_python_tmp = tmp_f.read()

		self.assertEqual(from_python_tmp, from_python[2:-2])

		os.remove(tmp_path)
		data_object_free(data_obj)

	def create_buffer(self):
		"Create a buffer used in testSegcolStore*"

		fd = get_file_fd("buffer_tests.py")
		
		# Read data using python functions
		f = os.fdopen(fd)

		from_python = f.read()

		r1 = len(from_python)/3
		r2 = len(from_python)/2
		
		f.close()
		fd = get_file_fd("buffer_tests.py")

		# Create some segments from the file and put them in a
		# segcol along with a memory segment
		(err, data_obj) = data_object_file_new(fd)
		self.assertEqual(err, 0)

		# Create and fill memory data object
		(err, data_obj1) = data_object_memory_new_ptr(bless_malloc(10), 10)
		self.assertEqual(err, 0)

		data = "#!LBBLSS!#"
		(err, buf) = data_object_get_data(data_obj1, 0, 10, DATA_OBJECT_WRITE)
		self.assertEqual(err, 0)
		buf[:] = data

		# Create segments
		(err, seg1) = segment_new_ptr(int(data_obj), 0, r1, None)
		self.assertEqual(err, 0)

		(err, seg2) = segment_new_ptr(int(data_obj1), 0, 10, None)
		self.assertEqual(err, 0)

		(err, seg3) = segment_new_ptr(int(data_obj), r1, r2, None)
		self.assertEqual(err, 0)

		# Append the segments to the list
		(err, segcol) = segcol_list_new()
		self.assertEqual(err, 0)

		segcol_append(segcol, seg1)
		segcol_append(segcol, seg2)
		segcol_append(segcol, seg3)

		# Create a bless buffer and set the segcol
		(err, buf) = bless_buffer_new()
		self.assertEqual(err, 0)
		set_buffer_segcol(buf, segcol)

		# Read data from buffer and compare it to data read from python
		read_data = create_string_buffer(r1 + r2 + 10)
		err = bless_buffer_read(buf, 0, read_data, 0, r1 + r2 + 10)
		self.assertEqual(err, 0)
		self.assertEqual(from_python[:r1] + data + from_python[r1:r1+r2],
				read_data.value)

		return (buf, segcol, fd, read_data.value)
		
	def check_buffer(self, buf, expected_data):
		"Check if the buffer contains the expected_data"

		# Read data from buffer and compare it to expected data
		err, buf_size = bless_buffer_get_size(buf)
		read_data = create_string_buffer(buf_size)
		err = bless_buffer_read(buf, 0, read_data, 0, buf_size)
		self.assertEqual(err, 0)

		self.assertEqual(expected_data, read_data.value)

	def testSegcolStoreInMemory(self):
		"Store the segcol data in memory data objects"

		buf, segcol, fd, data = self.create_buffer()
		err, buf_size = bless_buffer_get_size(buf)

		# Store in memory
		err = segcol_store_in_memory(segcol, 0, buf_size)
		self.assertEqual(err, 0)

		# Close buffer file to make sure segcol_store_in_memory worked 
		os.close(fd)

		# Read data from buffer and compare it to original data
		self.check_buffer(buf, data)

		bless_buffer_free(buf)	

	def testSegcolStoreInFile(self):
		"Store the segcol data in file data objects"

		buf, segcol, fd, data = self.create_buffer()
		err, buf_size = bless_buffer_get_size(buf)

		# Store in file
		err = segcol_store_in_file(segcol, 0, buf_size, "/tmp"); 
		self.assertEqual(err, 0)

		# Close buffer file to make sure segcol_store_in_file worked 
		os.close(fd)

		# Read data from buffer and compare it to original data
		self.check_buffer(buf, data)

		bless_buffer_free(buf)	
	
	def testSegcolAddCopy(self):
		"Copy data from a segcol into another"

		# Create and fill memory data object
		(err, data_obj) = data_object_memory_new_ptr(bless_malloc(10), 10)
		self.assertEqual(err, 0)

		data = "0123456789"
		(err, buf) = data_object_get_data(data_obj, 0, 10, DATA_OBJECT_WRITE)
		self.assertEqual(err, 0)
		buf[:] = data

		# Create segments
		(err, seg1) = segment_new_ptr(int(data_obj), 0, 4, None)
		self.assertEqual(err, 0)

		(err, seg2) = segment_new_ptr(int(data_obj), 4, 3, None)
		self.assertEqual(err, 0)

		(err, seg3) = segment_new_ptr(int(data_obj), 7, 3, None)
		self.assertEqual(err, 0)

		# Append the segments to the list
		(err, segcol) = segcol_list_new()
		self.assertEqual(err, 0)

		segcol_append(segcol, seg2)
		segcol_append(segcol, seg3)
		segcol_append(segcol, seg1)

		(err, segcol1) = segcol_list_new()
		self.assertEqual(err, 0)

		segcol_append(segcol1, segment_copy(seg3)[1])
		segcol_append(segcol1, segment_copy(seg1)[1])
		segcol_append(segcol1, segment_copy(seg2)[1])

		# Create a bless buffer and set the segcol
		(err, buf) = bless_buffer_new()
		self.assertEqual(err, 0)
		set_buffer_segcol(buf, segcol)

		# Read data from buffer and compare it to data read from python
		read_data = create_string_buffer(10)
		err = bless_buffer_read(buf, 0, read_data, 0, 10)
		self.assertEqual(err, 0)
		self.assertEqual("4567890123", read_data.value)

		err = segcol_add_copy(segcol, 5, segcol1)
		self.assertEqual(err, 0)

		err, segcol_size = segcol_get_size(segcol)
		self.assertEqual(err, 0)
		self.assertEqual(segcol_size, 20)

		read_data = create_string_buffer(20)
		err = bless_buffer_read(buf, 0, read_data, 0, 20)
		self.assertEqual(err, 0)
		self.assertEqual("45678789012345690123", read_data.value)

		segcol_free(segcol1)
		bless_buffer_free(buf)

if __name__ == '__main__':
	unittest.main()

