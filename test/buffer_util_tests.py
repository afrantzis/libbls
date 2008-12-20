import unittest
from ctypes import create_string_buffer
from ctypes import c_char_p
import os
from libbless import *

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

	def testSegcolStoreInMemory(self):
		"Store the segcol data in memory data objects"

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

		# Store in memory
		err = segcol_store_in_memory(segcol, 0, r1 + r2 + 10)
		self.assertEqual(err, 0)

		# close the file to make sure that we are reading only from memory
		os.close(fd)

		# Read data from buffer again and compare it to data read from python
		read_data = create_string_buffer(r1 + r2 + 10)
		err = bless_buffer_read(buf, 0, read_data, 0, r1 + r2 + 10)
		self.assertEqual(err, 0)

		self.assertEqual(from_python[:r1] + data + from_python[r1:r1+r2],
				read_data.value)

		


			

		

		



		

if __name__ == '__main__':
	unittest.main()

