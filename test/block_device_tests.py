import unittest
import errno
from ctypes import create_string_buffer
import os
import sys
import tempfile
import commands
from libbls import *

class BlockDeviceTests(unittest.TestCase):

	def setUp(self):
		# Make sure we are root
		if os.geteuid() != 0:
			self.privileged = False
			return
		
		self.privileged = True
		
		# Create loopback block device
		(tmp_fd, self.tmp_path) = tempfile.mkstemp();
		os.close(tmp_fd)

		commands.getoutput('dd if=/dev/zero of=%s count=1' % self.tmp_path)
		tmp_fd = os.open(self.tmp_path, os.O_WRONLY)
		os.write(tmp_fd, "1234567890")
		os.close(tmp_fd)

		self.dev_name = commands.getoutput('losetup -f')
		commands.getoutput('losetup %s %s' % (self.dev_name, self.tmp_path))

		# Create a buffer
		(err, self.buf) = bless_buffer_new()
		self.assertEqual(err, 0)

		# open the block device
		self.tmp_fd = os.open(self.dev_name, os.O_RDWR)
		self.assertNotEqual(self.tmp_fd, -1)

		self.tmp_fd_size = os.lseek(self.tmp_fd, 0, os.SEEK_END)

		# Append the device data to the buffer
		(err, self.src) = bless_buffer_source_file(self.tmp_fd, None)
		self.assertEqual(err, 0)

		err = bless_buffer_append(self.buf, self.src, 0, self.tmp_fd_size)
		self.assertEqual(err, 0)

	def tearDown(self):
		# Remove device and backing file
		if self.privileged == False:
			return

		err = bless_buffer_source_unref(self.src)
		bless_buffer_free(self.buf)

		os.close(self.tmp_fd)

		commands.getoutput('losetup -d %s' % self.dev_name)
		os.unlink(self.tmp_path)

	def testReadBlockDevice(self):
		"""Read from a block device"""

		if self.privileged == False:
			func_name = sys._getframe().f_code.co_name
			print "Skipping %s [must be root to run]" % func_name
			return

		# Read the buffer
		read_data = create_string_buffer(self.tmp_fd_size + 1)
		err = bless_buffer_read(self.buf, 0, read_data, 0, self.tmp_fd_size)
		self.assertEqual(err, 0)

		expected_data = "1234567890"
		for i in range(10):
			self.assertEqual(read_data[i], expected_data[i])

		for i in range(10, self.tmp_fd_size):
			self.assertEqual(read_data[i], '\0')


		# Try an out of range read
		err = bless_buffer_read(self.buf, 0, read_data, 0, self.tmp_fd_size + 1)
		self.assertEqual(err, errno.EINVAL)

	def testSaveBlockDevice(self):
		"""Save a buffer to a block device"""

		if self.privileged == False:
			func_name = sys._getframe().f_code.co_name
			print "Skipping %s [must be root to run]" % func_name
			return

		i = 5
		while i <= self.tmp_fd_size - 10:
			err = bless_buffer_delete(self.buf, i, 10)
			self.assertEqual(err, 0)
			err = bless_buffer_insert(self.buf, i, self.src, 0, 10)
			self.assertEqual(err, 0)
			i = i + 5

		last = 5 * int((self.tmp_fd_size - 10) / 5)

		# Check 
		read_data = create_string_buffer(self.tmp_fd_size)
		err = bless_buffer_read(self.buf, 0, read_data, 0, self.tmp_fd_size)
		self.assertEqual(err, 0)

		expected_data = "1234567890"
		for i in range(last + 5):
			self.assertEqual(read_data[i], expected_data[i%5])

		for i in range(last + 5, last + 10):
			self.assertEqual(read_data[i], expected_data[i%10])

		for i in range(last + 10, self.tmp_fd_size):
			self.assertEqual(read_data[i], '\0')

		# save file
		err = bless_buffer_save(self.buf, self.tmp_fd, None)
		self.assertEqual(err, 0)

		# recheck
		err = bless_buffer_read(self.buf, 0, read_data, 0, self.tmp_fd_size)
		self.assertEqual(err, 0)
		
		for i in range(last + 5):
			self.assertEqual(read_data[i], expected_data[i%5])

		for i in range(last + 5, last + 10):
			self.assertEqual(read_data[i], expected_data[i%10])

		for i in range(last + 10, self.tmp_fd_size):
			self.assertEqual(read_data[i], '\0')

	def testTryLargerSaveBlockDevice(self):
		"""Try to save to a block device a buffer larger that the device"""

		if self.privileged == False:
			func_name = sys._getframe().f_code.co_name
			print "Skipping %s [must be root to run]" % func_name
			return
		
		# Delete some bytes, add more that was deleted
		err = bless_buffer_delete(self.buf, 0, 10)
		self.assertEqual(err, 0)
		err = bless_buffer_insert(self.buf, 0, self.src, 0, 11)
		self.assertEqual(err, 0)
		
		# Try to save buffer
		err = bless_buffer_save(self.buf, self.tmp_fd, None)
		self.assertEqual(err, errno.ENOSPC)

if __name__ == '__main__':
	unittest.main()
