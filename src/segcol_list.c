/**
 * @file segcol_list.c
 *
 * List implementation of segcol_t
 *
 * @author Alexandros Frantzis
 */
#include <stdlib.h>

#include "segcol.h"
#include "segcol_internal.h"

/**
 * A node of a doubly linked list
 */
struct list_node {
	segment_t *segment;
	struct list_node *prev;
	struct list_node *next;
};


struct segcol_list_iter_impl {
	struct list_node *node;
	off_t mapping;
};

struct segcol_list_impl {
	struct list_node *head;
};

/* Forward declarations */

static int list_insert_before(struct list_node *p, struct list_node *q);
static int list_insert_after(struct list_node *p, struct list_node *q);

int segcol_list_new(segcol_t *segcol);
static int segcol_list_free(segcol_t *segcol);
static int segcol_list_insert(segcol_t *segcol, off_t offset, segment_t *seg); 
static segcol_t *segcol_list_delete(segcol_t *segcol, off_t offset, size_t length);
static segcol_iter_t *segcol_list_find(segcol_t *segcol, off_t offset);
static void *segcol_list_iter_new(segcol_t *segcol);
static int segcol_list_iter_next(segcol_iter_t *iter);
static int segcol_list_iter_is_valid(segcol_iter_t *iter);
static segment_t *segcol_list_iter_get_segment(segcol_iter_t *iter);
static off_t segcol_list_iter_get_mapping(segcol_iter_t *iter);
static int segcol_list_iter_free(segcol_iter_t *iter);


/**
 * Insert a node before another node in a list
 *
 * @param p the node to which the new noded is inserted before
 * @param q the node to insert
 *
 * @return the operation status code
 */
static int list_insert_before(struct list_node *p, struct list_node *q)
{
	if ((p == NULL)	|| (q == NULL))
		return -1;

	q->next = p;
	q->prev = p->prev;

	if (p->prev != NULL)
		p->prev->next = q;

	p->prev = q;

	return 0;
}

/**
 * Insert a node after another node in a list
 *
 * @param p the node to which the new noded is inserted after
 * @param q the node to insert
 *
 * @return the operation status code
 */
static int list_insert_after(struct list_node *p, struct list_node *q)
{
	if ((p == NULL)	|| (q == NULL))
		return -1;

	q->next = p->next;
	q->prev = p;

	if (p->next != NULL)
		p->next->prev = q;

	p->next = q;

	return 0;
}

/**
 * Creates a new segcol_t using a linked list implementation
 *
 * @return the operation status code
 */
int segcol_list_new(segcol_t *segcol)
{
	struct segcol_list_impl *impl = malloc(sizeof(struct segcol_list_impl));

	impl->head = NULL;

	/* Register functions */
	segcol_register_impl(segcol,
			impl,
			segcol_list_free,
			segcol_list_insert,
			segcol_list_delete,
			segcol_list_find,
			segcol_list_iter_new,
			segcol_list_iter_next,
			segcol_list_iter_get_segment,
			segcol_list_iter_get_mapping,
			segcol_list_iter_is_valid,
			segcol_list_iter_free
			);

	return 0;
}

static int segcol_list_free(segcol_t *segcol)
{
	struct segcol_list_impl *impl = (struct segcol_list_impl *) segcol_get_impl(segcol);
	struct list_node *node = impl->head;

	/* free list nodes */
	while (node != NULL) {
		struct list_node *next = node->next;
		free(node->segment);
		free(node);
		node = next;
	}

	free(impl);

	return 0;
}

static int segcol_list_insert(segcol_t *segcol, off_t offset, segment_t *seg) 
{
	struct segcol_list_impl *impl = (struct segcol_list_impl *) segcol_get_impl(segcol);

	/* find the segment that 'offset' is mapped to */
	segcol_iter_t *iter = segcol_list_find(segcol, offset);

	if (!segcol_list_iter_is_valid(iter))
			return -1;

	segment_t *pseg = segcol_list_iter_get_segment(iter);
	off_t mapping = segcol_list_iter_get_mapping(iter);

	struct segcol_list_iter_impl *iter_impl = 
		(struct segcol_list_iter_impl *) segcol_iter_get_impl(iter);

	struct list_node *pnode = iter_impl->node;
	
	/* create a list node containing the new segment */
	struct list_node *qnode = malloc(sizeof(struct list_node));
	qnode->segment = seg;

	/* 
	 * split the existing segment and insert the new segment
	 */
	
	/* where to split the existing segment */
	off_t split_index = offset - mapping;

	/* check if a split is actually needed or we just have to prepend 
	 * the new segment */
	if (split_index == 0) {
		list_insert_before(pnode, qnode);
	} else {
		segment_t *rseg = segment_split(pseg, split_index);
		
		struct list_node *rnode = malloc(sizeof(struct list_node));
		rnode->segment = rseg;

		list_insert_after(pnode, qnode);
		list_insert_after(qnode, rnode);
	}

	return 0;
}

static segcol_t *segcol_list_delete(segcol_t *segcol, off_t offset, size_t length)
{
	struct segcol_list_impl *impl = (struct segcol_list_impl *) segcol_get_impl(segcol);
}

static void * segcol_list_iter_new(segcol_t *segcol)
{
	struct segcol_list_impl *impl = (struct segcol_list_impl *) segcol_get_impl(segcol);
	struct segcol_list_iter_impl* iter_impl = malloc(sizeof(struct segcol_list_iter_impl));

	iter_impl->node = impl->head;
	iter_impl->mapping = 0;

	return iter_impl;
}

static segcol_iter_t *segcol_list_find(segcol_t *segcol, off_t offset)
{
	segcol_iter_t *iter = segcol_iter_new(segcol);

	/* linear search of list nodes */
	while (segcol_list_iter_is_valid(iter)) {
		segment_t *seg = segcol_list_iter_get_segment(iter);
		off_t mapping = segcol_list_iter_get_mapping(iter);
		size_t node_size = segment_get_size(seg);

		if ((offset >= mapping) && (offset < mapping + node_size))
			break;
	}

	/* 
	 * at this point we either have an invalid iter (search failed)
	 * or a valid iter that points to the correct node (search succeeded)
	 */
	return iter;
}

static int segcol_list_iter_next(segcol_iter_t *iter)
{
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);

	if (iter_impl->node != NULL) {
		size_t node_size = segment_get_size(iter_impl->node->segment);
		iter_impl->node = iter_impl->node->next;
		iter_impl->mapping = iter_impl->mapping + node_size;
	} 

	return 0;
}

static segment_t *segcol_list_iter_get_segment(segcol_iter_t *iter)
{
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
	
	return iter_impl->node->segment;
}

static off_t segcol_list_iter_get_mapping(segcol_iter_t *iter)
{
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
	
	return iter_impl->mapping;
}

static int segcol_list_iter_is_valid(segcol_iter_t *iter)
{
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);

	return (iter_impl != NULL) && (iter_impl->node != NULL);
}

static int segcol_list_iter_free(segcol_iter_t *iter)
{
	free(segcol_iter_get_impl(iter));
}

