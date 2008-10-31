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
#include "segcol_list.h"


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
	struct list_node *tail;
	struct list_node *cached_node;
	off_t cached_mapping;
};

/* Forward declarations */

/* List functions */
static int list_insert_before(struct list_node *p, struct list_node *q);
static int list_insert_after(struct list_node *p, struct list_node *q);
static int list_new_node(struct list_node **node);
static int list_delete_chain(struct list_node *start, struct list_node *end);

/* internal convenience functions */
static int find_node(segcol_t *segcol, 
		struct list_node **node, off_t *mapping, off_t offset);
static int segcol_list_clear_cache(struct segcol_list_impl *impl);
static int segcol_list_set_cache(struct segcol_list_impl *impl,
		struct list_node *node, off_t mapping);

/* segcol API implementation functions */
int segcol_list_new(segcol_t **segcol);
static int segcol_list_free(segcol_t *segcol);
static int segcol_list_append(segcol_t *segcol, segment_t *seg); 
static int segcol_list_insert(segcol_t *segcol, off_t offset, segment_t *seg); 
static int segcol_list_delete(segcol_t *segcol, segcol_t **deleted, off_t offset, off_t length);
static int segcol_list_find(segcol_t *segcol, segcol_iter_t **iter, off_t offset);
static int segcol_list_iter_new(segcol_t *segcol, void **iter);
static int segcol_list_iter_next(segcol_iter_t *iter);
static int segcol_list_iter_is_valid(segcol_iter_t *iter, int *valid);
static int segcol_list_iter_get_segment(segcol_iter_t *iter, segment_t **seg);
static int segcol_list_iter_get_mapping(segcol_iter_t *iter, off_t *mapping);
static int segcol_list_iter_free(segcol_iter_t *iter);

/* Function pointers for the list implementation of segcol_t */
static struct segcol_funcs segcol_list_funcs = {
	.free = segcol_list_free,
	.append = segcol_list_append,
	.insert = segcol_list_insert,
	.delete = segcol_list_delete,
	.find = segcol_list_find,
	.iter_new = segcol_list_iter_new,
	.iter_next = segcol_list_iter_next,
	.iter_is_valid = segcol_list_iter_is_valid,
	.iter_get_segment = segcol_list_iter_get_segment,
	.iter_get_mapping = segcol_list_iter_get_mapping,
	.iter_free = segcol_list_iter_free
};

/**
 * Inserts a node before another node in a list.
 *
 * @param p the node to which the new noded is inserted before
 * @param q the node to insert
 *
 * @return the operation error code
 */
static int list_insert_before(struct list_node *p, struct list_node *q)
{
	if ((p == NULL)	|| (q == NULL))
		return EINVAL;

	q->next = p;
	q->prev = p->prev;

	p->prev->next = q;

	p->prev = q;

	return 0;
}

/**
 * Inserts a node after another node in a list.
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
static int list_delete_chain(struct list_node *first, struct list_node *last)
{
	if (first == NULL || last == NULL)
		return EINVAL;

	struct list_node *prev_node = first->prev;
	struct list_node *next_node = last->next;

	next_node->prev = prev_node;

	prev_node->next = next_node;

	first->prev = NULL;
	last->next = NULL;

	return 0;
}

/**
 * Creates a new list_node.
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
	(*node)->segment = NULL;

	return 0;
}

/**
 * Finds the node in the segcol_list that contains a logical offset.
 * 
 * @param segcol_list the segcol_list to search
 * @param[out] node the found node (or NULL if not found)
 * @param[out] mapping the mapping of the found node
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
	} else {
		struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
		*node = iter_impl->node;
		*mapping = iter_impl->mapping;
	}

	segcol_iter_free(iter);

	return 0;
}

/**
 * Clears the search cache of a segcol_list_impl.
 *
 * @param impl the segcol_list_impl
 *
 * @return the operation error code
 */
static int segcol_list_clear_cache(struct segcol_list_impl *impl)
{
	if (impl == NULL)
		return EINVAL;

	impl->cached_node = NULL;
	impl->cached_mapping = 0;

	return 0;
}

/**
 * Sets the search cache of a segcol_list_impl.
 *
 * @param impl the segcol_list_impl
 * @param node the cached list node
 * @param mapping the logical mapping of the cached node in the segcol_list
 *
 * @return the operation error code
 */
static int segcol_list_set_cache(struct segcol_list_impl *impl,
		struct list_node *node, off_t mapping)
{
	if (impl == NULL || node == NULL)
		return EINVAL;

	impl->cached_node = node;
	impl->cached_mapping = mapping;

	return 0;
}


/*****************
 * API functions *
 *****************/

/**
 * Creates a new segcol_t using a linked list implementation.
 *
 * @param[out] segcol the created segcol_t
 *
 * @return the operation error code
 */
int segcol_list_new(segcol_t **segcol)
{
	if (segcol == NULL)
		return EINVAL;

	/* Allocate memory for implementation */
	struct segcol_list_impl *impl = malloc(sizeof(struct segcol_list_impl));
	
	if (impl == NULL)
		return EINVAL;

	/* Create head and tail nodes */
	int err = list_new_node(&impl->head);

	if (err) {
		free(impl);
		return err;
	}

	err = list_new_node(&impl->tail);

	if (err) {
		free(impl->head);
		free(impl);
		return err;
	}
	
	/* Connect head and tail nodes */
	impl->head->next = impl->tail;
	impl->head->prev = impl->head;

	impl->tail->next = impl->tail;
	impl->tail->prev = impl->head;

	segcol_list_clear_cache(impl);

	/* Create segcol_t */
	err = segcol_create_impl(segcol, impl, &segcol_list_funcs);

	if (err) {
		free(impl->tail);
		free(impl->head);
		free(impl);
		return err;
	}

	return 0;
}

static int segcol_list_free(segcol_t *segcol)
{
	if (segcol == NULL)
		return EINVAL;
		
	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	struct list_node *node = impl->head;
	struct list_node *next;
	struct list_node *cur;

	/* free list nodes */
	do {
		next = node->next;
		cur = node;

		if (node->segment != NULL)
			segment_free(node->segment);
		free(node);
		node = next;
	} while (next != cur);

	free(impl);

	return 0;
}

static int segcol_list_append(segcol_t *segcol, segment_t *seg) 
{
	if (segcol == NULL || seg == NULL)
		return EINVAL;

	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);
	
	struct list_node *new_node;
	int err = list_new_node(&new_node);
	if (err)
		return err;
		
	new_node->segment = seg;

	/* Find the node to append after (last node) */
	err = list_insert_before(impl->tail, new_node);
	
	return 0;
}

static int segcol_list_insert(segcol_t *segcol, off_t offset, segment_t *seg) 
{
	if (segcol == NULL || seg == NULL)
		return EINVAL;

	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	/* find the segment that 'offset' is mapped to */
	segcol_iter_t *iter;
	int err = segcol_list_find(segcol, &iter, offset);
	if (err)
		return err;

	int valid;
	if (segcol_list_iter_is_valid(iter, &valid) || !valid)
			return EINVAL;

	segcol_list_clear_cache(impl);

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
	if (qnode == NULL)
		return ENOMEM;

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
		segment_t *rseg;
		segment_split(pseg, &rseg, split_index);
		
		struct list_node *rnode = malloc(sizeof(struct list_node));
		rnode->segment = rseg;

		list_insert_after(pnode, qnode);
		list_insert_after(qnode, rnode);
	}

	return 0;
}

/*
 * The algorithm first deletes from the segcol list the node chain that is
 * defined by first_node and last_node (the nodes that contain the start and
 * end of the range, respectively). This possibly deletes more than it should
 * because we completely delete the first and last segment, when perhaps only a
 * part of them should be deleted (F, L). We fix this later on by reinserting
 * a and b. Some of the comments below will refer to the following diagram:
 *
 *  P: first_node_prev a: node_a F: first_node
 *  N: last_node_next b: node_b L: last_node
 *  
 *  -[P]-[a|F]-[]-[]-[L|b]-[N]-  => -[]-[P]-[N]-[]- => -[P]-[a]-[b]-[N]-
 *          ^           ^
 *        offset  offset + length
 */
static int segcol_list_delete(segcol_t *segcol, segcol_t **deleted, off_t
		offset, off_t length)
{ 
	if (segcol == NULL)
		return EINVAL;

	struct segcol_list_impl *impl = 
		(struct segcol_list_impl *) segcol_get_impl(segcol);


	struct list_node *first_node = NULL;
	struct list_node *last_node = NULL;
	off_t first_mapping;
	off_t last_mapping;

	/* Find the first and last list nodes that contain the range */
	int err = find_node(segcol, &first_node, &first_mapping, offset);
	if (err)
		return err;

	err = find_node(segcol, &last_node, &last_mapping, offset + length - 1);
	if (err)
		return err;

	if (first_node == NULL || last_node == NULL 
		|| first_node->segment == NULL || last_node->segment == NULL)
		return EINVAL;

	segcol_list_clear_cache(impl);

	/* 
	 * node_a will hold the part of the first segment that we must
	 * put back into the segcol.
	 * node_b will hold the part of the last segment that we must
	 * put back into the segcol.
	 */
	struct list_node *node_a;
	struct list_node *node_b;

	err = list_new_node(&node_a);
	if (err)
		return err;
	err = list_new_node(&node_b);
	if (err)
		return err;

	/* 
	 * The nodes that should go before and after node_a and node_b, respectively 
	 * when and if we should insert them back in.
	 */
	struct list_node *node_a_prev = first_node->prev;
	struct list_node *node_b_next = last_node->next;

    /* Delete the chain */
	err = list_delete_chain(first_node, last_node);
	if (err)
		return err;

	/* Calculate new size, after having deleted the chain */
	off_t new_size;
	segcol_get_size(segcol, &new_size);

	off_t last_seg_size;
	err = segment_get_size(last_node->segment, &last_seg_size);

	new_size -= last_mapping + last_seg_size - first_mapping;

	/* 
	 * Handle first node: Split first segment so that the first node contains
	 * only segment F (and store the remaining in node_a). We only have to do
	 * something if the range to delete starts after the beginning of the first
	 * segment.  Otherwise the whole segment should be deleted which has
	 * already been done by list_delete_chain.
	 */
	if (first_mapping < offset) {
		segment_t *tmp_seg;
		segment_split(first_node->segment, &tmp_seg, offset - first_mapping);

		node_a->segment = first_node->segment;
		first_node->segment = tmp_seg;
	} else {
		free(node_a);
		node_a = NULL;
	}

	/* 
	 * Handle last node: Split last segment so that the last node contains only
	 * segment L (and store the remaining in node_b). We only have to do
	 * something if the range to delete ends after the end of the last segment.
	 * Otherwise the whole segment should be deleted which has already been
	 * done by list_delete_chain.
	 */
	if (last_mapping + last_seg_size > offset + length) {
		/*
		 * If the first and last nodes are the same the segment that is
		 * contained in them may have been decreased by 'offset - mapping' due
		 * to the split in the handling of the first node. Take this into
		 * account to correctly calculate the index for the second split.
		 */
		int last_seg_dec = 0; if (first_node == last_node) last_seg_dec =
			offset - first_mapping;

		segment_t *tmp_seg;
		segment_split(last_node->segment, &tmp_seg,
				offset + length - last_mapping - last_seg_dec);
		node_b->segment = tmp_seg;
	} else {
		free(node_b);
		node_b = NULL;
	}

	/* 
	 * Insert incorrectly deleted parts of the segments back into segcol
	 */

	if (node_a != NULL) {
		err = list_insert_after(node_a_prev, node_a);

		if (err)
			return err;
	}

	if (node_b != NULL) {
		err = list_insert_before(node_b_next, node_b);

		if (err)
			return err;
	}

	/* Create a new segcol_t to put the deleted segments */
	/* TODO: Optimize: We can put the node chain as is in the new
	 * segcol_list. No need to add the segments one-by-one. */
	if (deleted != NULL)
		segcol_list_new(deleted);

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
	if (segcol == NULL)
		return EINVAL;

	int err;

	struct segcol_list_impl *impl = 
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	struct list_node *cur_node = impl->cached_node;
	off_t cur_mapping = impl->cached_mapping;

	if (cur_node == NULL) {
		cur_node = impl->head->next;
		cur_mapping = 0;
	}

	int fix_mapping = 0;

	/* linear search of list nodes */
	while (cur_node != cur_node->next && cur_node != cur_node->prev) {
		segment_t *seg = cur_node->segment;
		off_t seg_size;
		err = segment_get_size(seg, &seg_size);
		if (err)
			return err;

		/* 
		 * When we move backwards in the list the new mapping is the
		 * cur_mapping - prev_seg->size. In order to avoid getting the
		 * size twice we just set a flag to indicate that we should fix
		 * the mapping.
		 */
		if (fix_mapping)
			cur_mapping -= seg_size;

		/* We have found the node! */
		if (offset >= cur_mapping && offset < cur_mapping + seg_size) {
			segcol_list_set_cache(impl, cur_node, cur_mapping);
			break;
		}
		
		/* 
		 * Move forwards or backwards in the list depending on where
		 * is the offset relative to the current node.
		 */
		if (offset < cur_mapping) {
			cur_node = cur_node->prev;
			/* 
			 * Fix the mapping in the next iteration. Otherwise we would
			 * to get the size here, which is a waste since we are doing
			 * that at the start of the loop.
			 */
			fix_mapping = 1;
		}
		else if (offset >= cur_mapping + seg_size) {
			cur_node = cur_node->next;
			fix_mapping = 0;
			cur_mapping += seg_size;
		}
	}

	/* Create iterator to return search results */
	err = segcol_iter_new(segcol, iter);
	if (err)
		return err;
	
	struct segcol_list_iter_impl *iter_impl = 
		(struct segcol_list_iter_impl *) segcol_iter_get_impl(*iter);
	
	iter_impl->node = cur_node;
	iter_impl->mapping = cur_mapping;

	/* 
	 * at this point we either have an invalid iter (search failed)
	 * or a valid iter that points to the correct node (search succeeded)
	 */
	return 0;
}

static int segcol_list_iter_new(segcol_t *segcol, void **iter_impl)
{
	if (segcol == NULL || iter_impl == NULL)
		return EINVAL;

	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);
	struct segcol_list_iter_impl **iter_impl1 =
		(struct segcol_list_iter_impl **)iter_impl;

	*iter_impl1 = malloc(sizeof(struct segcol_list_iter_impl));

	(*iter_impl1)->node = impl->head->next;
	(*iter_impl1)->mapping = 0;

	return 0;
}


static int segcol_list_iter_next(segcol_iter_t *iter)
{
	if (iter == NULL)
		return EINVAL;
	
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);

	if (iter_impl->node != iter_impl->node->next) {
		off_t node_size;
		segment_get_size(iter_impl->node->segment, &node_size);
		iter_impl->node = iter_impl->node->next;
		iter_impl->mapping = iter_impl->mapping + node_size;
	} 

	return 0;
}

static int segcol_list_iter_get_segment(segcol_iter_t *iter, segment_t **seg)
{
	if (iter == NULL || seg == NULL)
		return EINVAL;

	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
	
	*seg = iter_impl->node->segment;

	return 0;
}

static int segcol_list_iter_get_mapping(segcol_iter_t *iter, off_t *mapping)
{
	if (iter == NULL || mapping == NULL)
		return EINVAL;

	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
	
	*mapping = iter_impl->mapping;

	return 0;
}

static int segcol_list_iter_is_valid(segcol_iter_t *iter, int *valid)
{
	if (iter == NULL || valid == NULL)
		return EINVAL;

	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);

	/* Iterator is valid if it is not NULL and its node its not the head 
	 * or tail */
	*valid = (iter_impl != NULL) && (iter_impl->node != iter_impl->node->next)
		&& (iter_impl->node != iter_impl->node->prev);

	return 0;
}

static int segcol_list_iter_free(segcol_iter_t *iter)
{
	if (iter == NULL)
		return EINVAL;

	free(segcol_iter_get_impl(iter));

	return 0;
}
