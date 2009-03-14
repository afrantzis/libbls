import unittest
import os
from libbls import *

class UtilTests(unittest.TestCase):

	def setUp(self):
		pass

	def tearDown(self):
		pass

	def check_path_join(self, s1, s2):
		err, p = path_join(s1, s2)
		self.assertEqual(err, 0)
	
		r = os.path.join(s1, s2)

		self.assertEqual(p, r)

	def testPathJoin(self):
		"Combine paths"

		self.check_path_join('/tmp', 'lb')
		self.check_path_join('/tmp/', 'lb')
		self.check_path_join('/tmp', '')
		self.check_path_join('/tmp/', '')

		
if __name__ == '__main__':
	unittest.main()

