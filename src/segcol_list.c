/**
 * @file segcol_list.c
 *
 * List implementation of segcol_t
 *
 * @author Alexandros Frantzis
 */
#include <stdlib.h>
#include <errno.h>

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

/* List functions */
static int list_insert_before(struct segcol_list_impl *impl,
		struct list_node *p, struct list_node *q);
static int list_insert_after(struct list_node *p, struct list_node *q);
static int list_new_node(struct list_node **node);
static int list_delete_chain(struct segcol_list_impl *impl, 
		struct list_node *start, struct list_node *end);

/* internal convenience functions */
static int find_node(segcol_t *segcol, 
		struct list_node **node, off_t *mapping, off_t offset);

/* segcol implementation functions */
int segcol_list_new(segcol_t *segcol);
static int segcol_list_free(segcol_t *segcol);
static int segcol_list_append(segcol_t *segcol, segment_t *seg); 
static int segcol_list_insert(segcol_t *segcol, off_t offset, segment_t *seg); 
static int segcol_list_delete(segcol_t *segcol, segcol_t **deleted, off_t offset, size_t length);
static int segcol_list_find(segcol_t *segcol, segcol_iter_t **iter, off_t offset);
static int segcol_list_iter_new(segcol_t *segcol, void **iter);
static int segcol_list_iter_next(segcol_iter_t *iter);
static int segcol_list_iter_is_valid(segcol_iter_t *iter, int *valid);
static int segcol_list_iter_get_segment(segcol_iter_t *iter, segment_t **seg);
static int segcol_list_iter_get_mapping(segcol_iter_t *iter, off_t *mapping);
static int segcol_list_iter_free(segcol_iter_t *iter);

/**
 * Inserts a node before another node in a list
 *
 * @param p the node to which the new noded is inserted before
 * @param q the node to insert
 *
 * @return the operation error code
 */
static int list_insert_before(struct segcol_list_impl *impl,
		struct list_node *p, struct list_node *q)
{
	if ((p == NULL)	|| (q == NULL))
		return EINVAL;

	q->next = p;
	q->prev = p->prev;

	if (p->prev != NULL)
		p->prev->next = q;
	else
		impl->head = q;

	p->prev = q;

	return 0;
}

/**
 * Inserts a node after another node in a list
 *
 * @param p the node to which the new noded is inserted after
 * @param q the node to insert
 *
 * @return the operation error code
 */
static int list_insert_after(struct list_node *p, struct list_node *q)
{
	if ((p == NULL)	|| (q == NULL))
		return EINVAL;

	q->next = p->next;
	q->prev = p;

	if (p->next != NULL)
		p->next->prev = q;

	p->next = q;

	return 0;
}

/**
 * Deletes a chain of nodes from the list.
 *
 * This operation doesn't free the memory
 * occupied by the nodes.
 *
 * @param impl the list to delete the chain from
 * @param first the first node in the chain to delete
 * @param last the last node in the chain to delete
 *
 * @return the operation error code
 */
static int list_delete_chain(struct segcol_list_impl *impl, 
		struct list_node *first, struct list_node *last)
{
	if (impl == NULL || first == NULL || last == NULL)
		return EINVAL;

	struct list_node *prev_node = first->prev;
	struct list_node *next_node = last->next;

	/* If the start node was the first in the list update the list head */
	if (prev_node == NULL)
		impl->head = next_node;

	if (next_node != NULL)
		next_node->prev = prev_node;

	if (prev_node != NULL)
		prev_node->next = next_node;

	first->prev = NULL;
	last->next = NULL;

	return 0;
}

/**
 * Creates a new list_node
 *
 * @param[out] node the new list node
 *
 * @return the operation error code
 */
static int list_new_node(struct list_node **node)
{
	*node = malloc(sizeof(struct list_node));

	if (*node == NULL)
		return ENOMEM;

	(*node)->prev = NULL;
	(*node)->next = NULL;

	return 0;
}

/**
 * Finds the node in the segcol_list that contains a logical offset.
 * 
 * @param segcol_list the segcol_list to search
 * @param[out] node the found node (or NULL if not found)
 * @param offset the offset to look for
 *
 * @return the operation error code
 */
static int find_node(segcol_t *segcol, 
		struct list_node **node, off_t *mapping, off_t offset)
{
	segcol_iter_t *iter = NULL;
	int err = segcol_find(segcol, &iter, offset);

	if (err)
		return err;

	int valid = 0;
	if (segcol_iter_is_valid(iter, &valid) || !valid) {
		*node = NULL;
	}
	else {
		struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
		*node = iter_impl->node;
		*mapping = iter_impl->mapping;
	}

	segcol_iter_free(iter);

	return 0;
}



/**
 * Creates a new segcol_t using a linked list implementation
 *
 * @return the operation error code
 */
int segcol_list_new(segcol_t *segcol)
{
	if (segcol == NULL)
		return EINVAL;

	struct segcol_list_impl *impl = malloc(sizeof(struct segcol_list_impl));

	impl->head = NULL;

	/* Register functions */
	segcol_register_impl(segcol,
			impl,
			segcol_list_free,
			segcol_list_append,
			segcol_list_insert,
			segcol_list_delete,
			segcol_list_find,
			segcol_list_iter_new,
			segcol_list_iter_next,
			segcol_list_iter_is_valid,
			segcol_list_iter_get_segment,
			segcol_list_iter_get_mapping,
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
		segment_free(node->segment);
		free(node);
		node = next;
	}

	free(impl);

	return 0;
}

static int segcol_list_append(segcol_t *segcol, segment_t *seg) 
{
	if (segcol == NULL || seg == NULL)
		return EINVAL;

	struct segcol_list_impl *impl = (struct segcol_list_impl *) segcol_get_impl(segcol);
	
	struct list_node *new_node;
	int err = list_new_node(&new_node);
	if (err)
		return err;
		
	new_node->segment = seg;

	/* Find the node to append after (last node) */
	struct list_node *n = impl->head;

	if (n != NULL)
		while (n->next != NULL)
			n = n->next;

	/* Check if this is the only node in the list */
	if (n == NULL)
		impl->head = new_node;
	else
		list_insert_after(n, new_node);
	
	return 0;
}

static int segcol_list_insert(segcol_t *segcol, off_t offset, segment_t *seg) 
{
	if (segcol == NULL || seg == NULL)
		return EINVAL;

	struct segcol_list_impl *impl = (struct segcol_list_impl *) segcol_get_impl(segcol);

	/* find the segment that 'offset' is mapped to */
	segcol_iter_t *iter;
	segcol_list_find(segcol, &iter, offset);

	int valid;
	if (segcol_list_iter_is_valid(iter, &valid) || !valid)
			return -1;

	segment_t *pseg;
	segcol_list_iter_get_segment(iter, &pseg);

	off_t mapping;
	segcol_list_iter_get_mapping(iter, &mapping);

	struct segcol_list_iter_impl *iter_impl = 
		(struct segcol_list_iter_impl *) segcol_iter_get_impl(iter);

	struct list_node *pnode = iter_impl->node;

	segcol_iter_free(iter);
	
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
		list_insert_before(impl, pnode, qnode);
		/* Update the head pointer if needed */
		if (pnode == impl->head)
			impl->head = qnode;
	} else {
		segment_t *rseg;
		segment_split(pseg, &rseg, split_index);
		
		struct list_node *rnode = malloc(sizeof(struct list_node));
		rnode->segment = rseg;

		list_insert_after(pnode, qnode);
		list_insert_after(qnode, rnode);
	}

	return 0;
}

static int segcol_list_delete(segcol_t *segcol, segcol_t **deleted, 
		off_t offset, size_t length)
{
	struct segcol_list_impl *impl = 
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	struct list_node *first_node = NULL;
	struct list_node *last_node = NULL;
	off_t first_mapping;
	off_t last_mapping;

	/* Find the first and last list nodes that contain the range */
	/* 
	 * TODO: Possibly optimize to avoid traversing the list twice.
	 * A caching segcol_list_find() would solve this.
	 */

	int err = find_node(segcol, &first_node, &first_mapping, offset);
	if (err)
		return err;

	err = find_node(segcol, &last_node, &last_mapping, offset + length - 1);
	if (err)
		return err;

	if (first_node == NULL || last_node == NULL)
		return EINVAL;


	/*
	 * Delete from the segcol list the node chain that is defined by
	 * first_node and last_node. This possibly deletes more than it
	 * should because we completely delete the first and last segment,
	 * when perhaps only a part of them should be deleted. We fix this
	 * later on (see seg_a and seg_b). Some of the comments below will
	 * refer to the following diagram:
	 *
	 *           first          last
	 *  []-[]-[]-[a|X]-[]-[]-[]-[Y|b]-[]-[]
	 *              ^              ^
	 *            offset    offset + length
	 */
	err = list_delete_chain(impl, first_node, last_node);
	if (err)
		return err;

	/* Calculate new size, after having deleted the chain */
	size_t new_size;
	segcol_get_size(segcol, &new_size);

	size_t last_seg_size;
	segment_get_size(last_node->segment, &last_seg_size);

	new_size -= last_mapping + last_seg_size - first_mapping;

	/* 
	 * seg_a will hold the part of the first segment that we must
	 * put back into the segcol.
	 * seg_b will hold the part of the last segment that we must
	 * put back into the segcol.
	 * Either one may be empty.
	 */
	segment_t *seg_a = NULL;
	segment_t *seg_b = NULL;

	/* 
	 * Handle first node: Split first segment so that the first node contains
	 * only segment X (and store seg_a). We only have to do something if the
	 * range to delete starts after the beginning of the first segment.
	 * Otherwise the whole segment should be deleted which has already been done
	 * by list_delete_chain.
	 */
	if (first_mapping < offset) {
		segment_t *tmp_seg;
		segment_split(first_node->segment, &tmp_seg, offset - first_mapping);

		seg_a = first_node->segment;
		first_node->segment = tmp_seg;
	}

	/* 
	 * Handle last node: Split last segment so that the last node contains only
	 * segment Y (and store seg_b). We only have to do something if the range
	 * to delete ends after the end of the last segment.  Otherwise the whole
	 * segment should be deleted which has already been done by list_delete_chain.
	 */
	if (last_mapping + last_seg_size > offset + length) {
		/*
		 * If the first and last nodes are the same the segment that is
		 * contained in them may have been decreased by 'offset - mapping'
		 * due to the split in the handling of the first node. Take this
		 * into account to correctly calculate the index for the second split.
		 */
		int last_seg_dec = 0;
		if (first_node == last_node)
			last_seg_dec = offset - first_mapping;

		segment_t *tmp_seg;
		segment_split(last_node->segment, &tmp_seg,
				offset + length - last_mapping - last_seg_dec);
		seg_b = tmp_seg;
	}

	/* 
	 * Insert incorrectly deleted parts of the segments back into segcol
	 */

	if (seg_a != NULL) {
		/* Decide whether to insert or append */
		if (first_mapping < new_size)
			err = segcol_list_insert(segcol, first_mapping, seg_a);
		else if (first_mapping == new_size)
			err = segcol_list_append(segcol, seg_a);
		else
			err = -1;

		if (err)
			return err;
	}

	if (seg_b != NULL) {
		off_t ins_off = first_mapping;

		/* 
		 * If seg_a has been inserted,
		 * seg_b must be placed after seg_a 
		 */
		if (seg_a != NULL) {
			size_t seg_a_size;
			segment_get_size(seg_a, &seg_a_size);
			new_size += seg_a_size;
			ins_off += seg_a_size;
		}

		/* Decide whether to insert or append */
		if (ins_off < new_size)
			err = segcol_list_insert(segcol, ins_off, seg_b);
		else if (ins_off == new_size)
			err = segcol_list_append(segcol, seg_b);
		else
			err = -1;
			
		if (err)
			return err;
	}

	/* Create a new segcol_t to put the deleted segments */
	/* TODO: Optimize: We can put the node chain as is in the new
	 * segcol_list. No need to add the segments one-by-one. */
	if (deleted != NULL)
		segcol_new(deleted, "list");

	struct list_node *n = first_node;

	while(n != last_node->next) {
		segcol_append(*deleted, n->segment);
		struct list_node *next_node = n->next;
		free(n);
		n = next_node;
	}

	return 0;
}

static int segcol_list_find(segcol_t *segcol, segcol_iter_t **iter, off_t offset)
{
	segcol_iter_new(segcol, iter);

	int valid = 0;

	/* linear search of list nodes */
	while (!segcol_list_iter_is_valid(*iter, &valid) && valid) {
		segment_t *seg;
		segcol_list_iter_get_segment(*iter, &seg);

		off_t mapping;
		segcol_list_iter_get_mapping(*iter, &mapping);

		size_t node_size;
		segment_get_size(seg, &node_size);

		if ((offset >= mapping) && (offset < mapping + node_size))
			break;

		segcol_iter_next(*iter);
	}

	/* 
	 * at this point we either have an invalid iter (search failed)
	 * or a valid iter that points to the correct node (search succeeded)
	 */
	return 0;
}

static int segcol_list_iter_new(segcol_t *segcol, void **iter_impl)
{
	struct segcol_list_impl *impl = (struct segcol_list_impl *) segcol_get_impl(segcol);
	struct segcol_list_iter_impl **iter_impl1 = (struct segcol_list_iter_impl **)iter_impl;

	*iter_impl1 = malloc(sizeof(struct segcol_list_iter_impl));

	(*iter_impl1)->node = impl->head;
	(*iter_impl1)->mapping = 0;

	return 0;
}


static int segcol_list_iter_next(segcol_iter_t *iter)
{
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);

	if (iter_impl->node != NULL) {
		size_t node_size;
		segment_get_size(iter_impl->node->segment, &node_size);
		iter_impl->node = iter_impl->node->next;
		iter_impl->mapping = iter_impl->mapping + node_size;
	} 

	return 0;
}

static int segcol_list_iter_get_segment(segcol_iter_t *iter, segment_t **seg)
{
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
	
	*seg = iter_impl->node->segment;

	return 0;
}

static int segcol_list_iter_get_mapping(segcol_iter_t *iter, off_t *mapping)
{
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
	
	*mapping = iter_impl->mapping;

	return 0;
}

static int segcol_list_iter_is_valid(segcol_iter_t *iter, int *valid)
{
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);

	*valid = (iter_impl != NULL) && (iter_impl->node != NULL);

	return 0;
}

static int segcol_list_iter_free(segcol_iter_t *iter)
{
	free(segcol_iter_get_impl(iter));

	return 0;
}
