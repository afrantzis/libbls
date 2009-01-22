import unittest
from libbls import *

class DisjointSetTests(unittest.TestCase):

	def setUp(self):
		(err, self.ds) = disjoint_set_new(10)
		self.assertEqual(err, 0)

	def tearDown(self):
		disjoint_set_free(self.ds)

	def testNew(self):
		for i in range(10):
			(err, set1) = disjoint_set_find(self.ds, i)
			self.assertEqual(set1, i)

	def testUnion(self):
		"Unite sets and check"

		for i in range(5):
			err = disjoint_set_union(self.ds, 2 * i, 2 * i + 1)
			self.assertEqual(err, 0)

		for i in range(5):
			(err, set1) = disjoint_set_find(self.ds, 2 * i)
			self.assertEqual(err, 0)
			(err, set2) = disjoint_set_find(self.ds, 2 * i + 1)
			self.assertEqual(err, 0)
			self.assertEqual(set1, set2)

			if i < 4:
				(err, set3) = disjoint_set_find(self.ds, 2 * i + 2)
				self.assertEqual(err, 0)
				self.assertNotEqual(set1, set3)

		err = disjoint_set_union(self.ds, 1, 2)
		self.assertEqual(err, 0)
		(err, set1) = disjoint_set_find(self.ds, 0)
		self.assertEqual(err, 0)
		(err, set2) = disjoint_set_find(self.ds, 3)
		self.assertEqual(err, 0)
		(err, set3) = disjoint_set_find(self.ds, 5)
		self.assertEqual(err, 0)
		self.assertEqual(set1, set2)
		self.assertNotEqual(set1, set3)

		err = disjoint_set_union(self.ds, 6, 9)
		self.assertEqual(err, 0)
		(err, set1) = disjoint_set_find(self.ds, 7)
		self.assertEqual(err, 0)
		(err, set2) = disjoint_set_find(self.ds, 8)
		self.assertEqual(err, 0)
		(err, set3) = disjoint_set_find(self.ds, 1)
		self.assertEqual(err, 0)
		self.assertEqual(set1, set2)
		self.assertNotEqual(set1, set3)
		
	def testInvalid(self):
		"Try to call functions with invalid elements"

		err = disjoint_set_union(self.ds, 1, 10)
		self.assertNotEqual(err, 0)
		
		err = disjoint_set_union(self.ds, 10, 0)
		self.assertNotEqual(err, 0)

		(err, set1) = disjoint_set_find(self.ds, 10)
		self.assertNotEqual(err, 0)
if __name__ == '__main__':
	unittest.main()
