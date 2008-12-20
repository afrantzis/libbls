import unittest
from libbless import *

class PriorityQueueTests(unittest.TestCase):

	def setUp(self):
		(err, self.pq) = priority_queue_new(10)
		self.assertEqual(err, 0)

	def tearDown(self):
		priority_queue_free(self.pq)

	def testNew(self):
		"Create a priority queue"

		self.assertNotEqual(self.pq, None)

		(err, size) = priority_queue_get_size(self.pq)
		self.assertEqual(err, 0)
		self.assertEqual(size, 0)

	def testAddRemove(self):
		"Add objects to the priority queue"

		data = "DJBCAEFIHG"

		for c in data:
			priority_queue_add(self.pq, c, ord(c), None)

		result_list = []

		while priority_queue_get_size(self.pq)[1] != 0:
			(err, c) = priority_queue_remove_max(self.pq)
			result_list.append(c)

		result = ''.join(result_list)

		self.assertEqual(result, "JIHGFEDCBA")

	def testAddRemoveExtend(self):
		"""Add objects to the priority queue so that it needs more
		memory than the initially allocated"""

		data = "NDJBCLAEFIKMHG"

		for c in data:
			priority_queue_add(self.pq, c, ord(c), None)

		result_list = []

		while priority_queue_get_size(self.pq)[1] != 0:
			(err, c) = priority_queue_remove_max(self.pq)
			result_list.append(c)

		result = ''.join(result_list)

		self.assertEqual(result, "NMLKJIHGFEDCBA")

	def testAddRemoveNegative(self):
		"""Add objects to the priority queue with negative priorities"""

		data = "NDJBCLAEFIKMHG"

		for c in data:
			priority_queue_add(self.pq, c, -ord(c), None)

		result_list = []

		while priority_queue_get_size(self.pq)[1] != 0:
			(err, c) = priority_queue_remove_max(self.pq)
			result_list.append(c)

		result = ''.join(result_list)

		self.assertEqual(result, "ABCDEFGHIJKLMN")

	def testChangeKeyDown(self):
		"Decrease the priority of an element in the priority queue"

		data = "NDJBCLAEFIKMHG"

		elem_pos = []
			
		for c in data:
			x = size_tp();
			elem_pos.append(x)
			priority_queue_add(self.pq, c, ord(c), x)

		# Reverse the priorities of the elements
		for i in range(len(data)):
			priority_queue_change_key(self.pq, elem_pos[i].value(),
					-ord(data[i]))
			
		result_list = []

		while priority_queue_get_size(self.pq)[1] != 0:
			(err, c) = priority_queue_remove_max(self.pq)
			result_list.append(c)

		result = ''.join(result_list)

		self.assertEqual(result, "ABCDEFGHIJKLMN")

	def testChangeKeyUp(self):
		"Increase the priority of an element in the priority queue"

		data = "NDJBCLAEFIKMHG"

		elem_pos = []
			
		for c in data:
			x = size_tp();
			elem_pos.append(x)
			priority_queue_add(self.pq, c, -ord(c), x)

		# Reverse the priorities of the elements
		for i in range(len(data)):
			priority_queue_change_key(self.pq, elem_pos[i].value(),
					ord(data[i]))
			
		result_list = []

		while priority_queue_get_size(self.pq)[1] != 0:
			(err, c) = priority_queue_remove_max(self.pq)
			result_list.append(c)

		result = ''.join(result_list)

		self.assertEqual(result, "NMLKJIHGFEDCBA")
		
if __name__ == '__main__':
	unittest.main()
