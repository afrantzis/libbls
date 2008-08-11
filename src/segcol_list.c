/**
 * @file segcol_list.c
 *
 * List implementation of segcol_t
 *
 * @author Alexandros Frantzis
 */
#include "segcol.h"
#include "segcol_internal.h"

/**
 * A node of doubly linked list
 */
struct list_node {
	segment_t *segment;
	struct list_node *prev;
	struct list_node *next;
};


static struct segcol_list_iter_impl {
		struct list_node *node;
		off_t mapping;
};

static struct segcol_list_impl {
	struct list_node *head;
};


/**
 * Creates a new segcol_t using a linked list implementation
 *
 * @return the new segcol_t
 */
segcol_t *segcol_list_new()
{
	struct segcol_list_impl *impl = malloc(sizeof(struct segcol_list_impl));

	segcol_t *segcol = malloc(sizeof(segcol_t));

	impl->head = NULL;

	segcol->impl = impl;

	segcol->insert = segcol_list_insert;
	segcol->delete = segcol_list_delete;
	segcol->find = segcol_list_find;
	segcol->iter_next = segcol_list_iter_next;
	segcol->iter_get_segment = segcol_list_iter_get_segment;

	return segcol;
}

static int segcol_list_free(void *impl)
{
	list_node *node = impl->head;

	/* free list nodes */
	while (node != NULL) {
		list_node *next = node->next;
		free(node->segment);
		free(node);
		node = next;
	}

	free(impl);

	return 0;
}

static int segcol_list_insert(void *impl, off_t offset, segment_t *seg) 
{
	struct segcol_list_impl *impl = (segcol_list_impl *)impl;

	/* find the segment that 'offset' is mapped to */
	segcol_iter_t *iter = segcol_list_find(impl, offset);

	if (!segcol_list_iter_is_valid(iter))
			return -1;

	segment_t *pseg = segcol_list_iter_get_segment(iter);
	off_t mapping = segcol_list_iter_get_mapping(iter);

	list_node *pnode = iter->node;
	
	/* create a list node containing the new segment */
	list_node *qnode = malloc(sizeof(list_node));
	qnode->segment = seg;

	/* split the existing segment and insert the new segment */
	
	/* where to split the existing segment */
	off_t split_index = offset - mapping;

	/* check if a split is actually needed or we just have to prepend 
	 * the new segment */
	if (split_index == 0) {
		list_insert_before(pnode, qnode);
	} else {
		segment_t *rseg = segment_split(pseg, split_index);
		
		list_node *rnode = malloc(sizeof(list_node));
		rnode->segment = rseg;

		list_insert_after(pnode, qnode);
		list_insert_after(qnode, pnode);
	}

	return 0;
}

static void segcol_list_delete(void *impl, off_t offset, size_t length);
{
}

static segcol_iter_t *segcol_list_find(void *impl, off_t offset);
{
}

static int segcol_list_iter_next(segcol_iter *iter);
{
}

static segment_t *segcol_list_iter_get_segment(segcol_iter_t *iter);
{
	struct segcol_list_iter_impl *iter_impl = iter->impl;
	
	return iter_impl->node->data;
}

static segment_t *segcol_list_iter_get_mapping(segcol_iter_t *iter);
{
	struct segcol_list_iter_impl *iter_impl = iter->impl;
	
	return iter_impl->node->mapping;
}

static int segcol_list_iter_is_valid(segcol_iter *iter)
{
}

static int segcol_list_iter_free(segcol_iter_t *iter)
{
		free(iter);
}

