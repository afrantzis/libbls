/**
 * @file priority_queue.c
 *
 * Priority queue implementation
 */

#include "priority_queue.h"
#include "type_limits.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>


/**
 * An element in the priority queue.
 */
struct element {
	void *data; /**< The data this element holds */
	int key; /**< The priority key of this element */
	size_t *pos; /**< Place to store the current position in the heap */ 
};

struct priority_queue {
	struct element *heap;
	size_t capacity;
	size_t size;
};

/* Forward declarations */
static int upheap(priority_queue_t *pq, size_t n);
static int downheap(priority_queue_t *pq, size_t n);

/********************/
/* Helper functions */
/********************/

static inline void place_element(struct element *h, size_t pos,
        struct element e)
{
	h[pos] = e;
	if (h[pos].pos != NULL)
		*h[pos].pos = pos;
}

/** 
 * Restore the heap property by moving an element upwards.
 * 
 * @param pq the priority queue
 * @param n the index of the element to move upwards
 * 
 * @return the operation error code
 */
static int upheap(priority_queue_t *pq, size_t n)
{
	struct element *h = pq->heap;
	int i = n;

	/* Store the Element */
	struct element k = h[n];
	
	/* 
	 * Move up the heap while the parent of the current
	 * element has less priority than the Element. On
	 * the way move the elements that we meet downwards.
	 *
	 * We have a sentinel at h[0] so there is no need
	 * to check for i > 1.
	 */
	while (k.key > h[i / 2].key) {
		place_element(h, i, h[i / 2]);
		i = i / 2;
	}

	/* Place the Element at its proper place */
	place_element(h, i, k);

	return 0;
}

/** 
 * Restore the heap property by moving an element downwards.
 * 
 * @param pq the priority queue
 * @param n the index of the element to move downwards
 * 
 * @return the operation error code
 */
static int downheap(priority_queue_t *pq, size_t n)
{
	struct element *h = pq->heap;

	size_t i = n;

	/* Store the element */
	struct element k = h[n];
	size_t pq_size = pq->size;


	/* While the current element has at least one child */
	while (2 * i <= pq_size) { 
		size_t j = 2 * i;

		/* 
		 * If there is a right child (j < pq_size) and the right 
		 * child (j + 1) is larger than the left (j), use it.
		 */
		if (j < pq_size && h[j].key < h[j + 1].key) j++;

		/* if the Element is larger than its children, stop the descent. */
		if (k.key > h[j].key) break;

		/* Move the current element upwards */
		place_element(h, i, h[j]);

		/* Move to chosen child */
		i = j;
	}

	/* Place the Element at its proper place */
	place_element(h, i, k);

	return 0;
}

/*****************/
/* API functions */
/*****************/

/** 
 * Creates a new priority queue.
 * 
 * @param[out] pq the created priority queue
 * @param capacity the initial capacity of the priority queue 
 * 
 * @return the operation error code
 */
int priority_queue_new(priority_queue_t **pq, size_t capacity)
{
	if (pq == NULL)
		return EINVAL;

	priority_queue_t *p = malloc(sizeof *p);

	if (p == NULL)
		return ENOMEM;

	/* 
	 * We need capacity + 1 because we start from index 1 in the
	 * heap in order for parent-children arithmetic relationships
	 * to work correctly.
	 */
	p->heap = malloc((capacity + 1) * sizeof *p->heap);

	if (p->heap == NULL) {
		free(p);
		return ENOMEM;
	}

	p->capacity = capacity;
	p->size = 0;

	/* Create sentinel element at index 0 */
	p->heap[0].key = __MAX(int);
	p->heap[0].data = NULL;

	*pq = p;

	return 0;
}

/** 
 * Frees a priority queue.
 *
 * This operation does not free the data added to it
 * by priority_queue_add().
 * 
 * @param pq the priority queue to free 
 * 
 * @return the operation error code
 */
int priority_queue_free(priority_queue_t *pq)
{
	if (pq == NULL)
		return EINVAL;

	free(pq->heap);
	free(pq);

	return 0;
}


/** 
 * Gets the size of the priority queue.
 * 
 * @param pq the priority queue 
 * @param[out] size the number of elements contained in the priority queue
 * 
 * @return the operation error code
 */
int priority_queue_get_size(priority_queue_t *pq, size_t *size)
{
	if (pq == NULL || size == NULL)
		return EINVAL;

	*size = pq->size;

	return 0;
}

/** 
 * Adds an element to the priority queue.
 * 
 * @param pq the priority queue to add the element to
 * @param data the element to be added 
 * @param key the priority key of the element to be added
 * @param pos where to store the current position of the element in the heap
 * 
 * @return the operation error code
 */
int priority_queue_add(priority_queue_t *pq, void *data, int key, size_t *pos)
{
	if (pq == NULL)
		return EINVAL;

	/* Allocate more space if needed */
	if (pq->size >= pq->capacity) {
		size_t new_size = (5 * pq->capacity) / 4;
		struct element *t = realloc(pq->heap, (new_size + 1) * sizeof *t);
		if (t == NULL)
			return ENOMEM;

		pq->heap = t;
		pq->capacity = new_size;
	}
	
	/* Place the new element at the end of the heap */
	pq->heap[++pq->size].data = data;
	pq->heap[pq->size].key = key;
	pq->heap[pq->size].pos = pos;

	/* Fix the heap property */
	upheap(pq, pq->size);

	return 0;
}

/** 
 * 
 * Remove the element with the maximum priority from the priority queue.
 * 
 * @param pq the priority queue to remove the element from 
 * @param[out] data the element with the maximum priority
 * 
 * @return the operation error code
 */
int priority_queue_remove_max(priority_queue_t *pq, void **data)
{
	if (pq == NULL || data == NULL)
		return EINVAL;

	/* Store the maximum element */
	struct element k = pq->heap[1];
	
	/* Move the last element to the top */
	place_element(pq->heap, 1, pq->heap[pq->size]);

	pq->size--;

	/* Fix the heap property */
	downheap(pq, 1);

	*data = k.data;

	return 0;
}

/** 
 * Change the priority key of an element.
 * 
 * @param pq the priority queue the element is in
 * @param pos the position of the element in the priority queue 
 * @param key the new key 
 * 
 * @return 
 */
int priority_queue_change_key(priority_queue_t *pq, size_t pos, int key)
{
	if (pq == NULL || pos > pq->size)
		return EINVAL;

	struct element *e = &pq->heap[pos];
	int old_key = e->key;

	e->key = key;

	if (key < old_key)
		downheap(pq, pos);
	else if (key > old_key)
		upheap(pq, pos);

	return 0;
}
