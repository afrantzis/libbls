/*
 * Copyright 2008, 2009 Alexandros Frantzis, Michael Iatrou
 *
 * This file is part of libbls.
 *
 * libbls is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * libbls is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file segcol_list.c
 *
 * List implementation of segcol_t
 */
#include <stdlib.h>
#include <errno.h>

#include "segcol.h"
#include "segcol_internal.h"
#include "segcol_list.h"
#include "type_limits.h"
#include "list.h"
#include "util.h"

/* Helper macros for list */
#define seg_list_head(ptr) list_head((ptr), struct segment_entry, ln)
#define seg_list_tail(ptr) list_tail((ptr), struct segment_entry, ln)

struct segment_entry {
	struct list_node ln;
	segment_t *segment;
};

struct segcol_list_iter_impl {
	struct list_node *node;
	off_t mapping;
};

struct segcol_list_impl {
	struct list *list;
	struct list_node *cached_node;
	off_t cached_mapping;
};

/* Forward declarations */

/* internal convenience functions */
static int find_seg_entry(segcol_t *segcol, 
		struct segment_entry **snode, off_t *mapping, off_t offset);
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
 * Finds the segment entry in the segcol_list that contains a logical offset.
 * 
 * @param segcol the segcol_t to search
 * @param[out] entry the found entry (or NULL if not found)
 * @param[out] mapping the mapping of the found node
 * @param offset the offset to look for
 *
 * @return the operation error code
 */
static int find_seg_entry(segcol_t *segcol, 
		struct segment_entry **entry, off_t *mapping, off_t offset)
{
	segcol_iter_t *iter = NULL;
	int err = segcol_find(segcol, &iter, offset);

	if (err)
		return_error(err);

	int valid = 0;
	if (segcol_iter_is_valid(iter, &valid) || !valid) {
		*entry = NULL;
	} else {
		struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
		*entry = list_entry(iter_impl->node, struct segment_entry, ln);
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
		return_error(EINVAL);

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
		return_error(EINVAL);

	impl->cached_node = node;
	impl->cached_mapping = mapping;

	return 0;
}

/*
 * Gets the closest known node/mapping pair to an offset.
 *
 * @param impl the segcol_list_impl
 * @param node the closest known list node
 * @param mapping the mapping of the closest known node in the segcol_list
 * @param offset the offset to look for
 *
 * @return the operation error code
 */
static int segcol_list_get_closest_node(segcol_t *segcol,
		struct list_node **node, off_t *mapping, off_t offset)
{
	off_t segcol_size;
	segcol_get_size(segcol, &segcol_size);

	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	/* 
	 * Calculate the distance of the head, tail and cached nodes to the offset.
	 * The distance metric is the byte distance of the offset to the
	 * afforementioned nodes' mappings. However, our search algorithm is
	 * O(#segments) not O(#byte_diff), so this metric may not lead to good
	 * results in some cases.
	 *
	 * Eg if the distance metric from the head is 10000 with 1000 intervening
	 * segment nodes and the metric from the tail is 100000 with 1 intervening
	 * segment node we will incorrectly choose head as the closest node.
	 */
	off_t dist_from_cache = __MAX(off_t);
	if (impl->cached_node != NULL) {
		*node = impl->cached_node;
		*mapping = impl->cached_mapping;
		if (offset > *mapping)
			dist_from_cache = offset - *mapping; 
		else
			dist_from_cache = *mapping - offset;
	}

	off_t dist_from_head = offset;
	off_t dist_from_tail = segcol_size - offset;

	/*
	 * Decide which is the closest node to the offset:
	 * head, cached or tail.
	 */
	off_t cur_min = dist_from_cache;

	if (dist_from_head < cur_min) {
		*node = seg_list_head(impl->list)->next;
		*mapping = 0;
		cur_min = dist_from_head;
	}

	if (dist_from_tail < cur_min) {
		*node = seg_list_tail(impl->list)->prev;

		struct segment_entry *snode =
			list_entry(*node, struct segment_entry, ln);
		off_t seg_size;
		segment_get_size(snode->segment, &seg_size);

		*mapping = segcol_size - seg_size;
	}

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
		return_error(EINVAL);

	/* Allocate memory for implementation */
	struct segcol_list_impl *impl = malloc(sizeof(struct segcol_list_impl));
	
	if (impl == NULL)
		return_error(EINVAL);

	/* Create head and tail nodes */
	int err = list_new(&impl->list, struct segment_entry, ln);
	if (err)
		return_error(err);

	segcol_list_clear_cache(impl);

	/* Create segcol_t */
	err = segcol_create_impl(segcol, impl, &segcol_list_funcs);

	if (err) {
		list_free(impl->list, struct segment_entry, ln);
		free(impl);
		return_error(err);
	}

	return 0;
}

static int segcol_list_free(segcol_t *segcol)
{
	if (segcol == NULL)
		return_error(EINVAL);
		
	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	struct list_node *node;

	/* free segments */
	list_for_each(seg_list_head(impl->list)->next, node) {
		struct segment_entry *snode = list_entry(node, struct segment_entry, ln);
		if (snode->segment != NULL)
			segment_free(snode->segment);
	}

	list_free(impl->list, struct segment_entry, ln);

	free(impl);

	return 0;
}

static int segcol_list_append(segcol_t *segcol, segment_t *seg) 
{
	if (segcol == NULL || seg == NULL)
		return_error(EINVAL);

	/* 
	 * If the segment size is 0, return successfully without adding
	 * anything. Free the segment as we will not be using it.
	 */
	off_t seg_size;
	segment_get_size(seg, &seg_size);
	if (seg_size == 0) {
		segment_free(seg);
		return 0;
	}

	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);
	
	struct segment_entry *new_node;
	int err = list_new_entry(&new_node, struct segment_entry, ln);
	if (err)
		return_error(err);
		
	new_node->segment = seg;

	/* Append at the end */
	list_insert_before(list_tail(impl->list, struct segment_entry, ln),
			&new_node->ln);
	
	/* Set the cache at the appended node */
	off_t segcol_size;
	segcol_get_size(segcol, &segcol_size);
	segcol_list_set_cache(impl, &new_node->ln, segcol_size);

	return 0;
}

static int segcol_list_insert(segcol_t *segcol, off_t offset, segment_t *seg) 
{
	if (segcol == NULL || seg == NULL || offset < 0)
		return_error(EINVAL);

	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	/* find the segment that 'offset' is mapped to */
	segcol_iter_t *iter;
	int err = segcol_list_find(segcol, &iter, offset);
	if (err)
		return_error(err);

	/* 
	 * If the segment size is 0, return successfully without adding
	 * anything. Free the segment as we will not be using it.
	 * This check is placed after the first find_seg_entry() so that the
	 * validity of offset (if it is in range) is checked first.
	 */
	off_t seg_size;
	segment_get_size(seg, &seg_size);
	if (seg_size == 0) {
		segment_free(seg);
		segcol_iter_free(iter);
		return 0;
	}

	segcol_list_clear_cache(impl);

	segment_t *pseg;
	segcol_list_iter_get_segment(iter, &pseg);

	off_t mapping;
	segcol_list_iter_get_mapping(iter, &mapping);

	struct segcol_list_iter_impl *iter_impl = 
		(struct segcol_list_iter_impl *) segcol_iter_get_impl(iter);

	struct segment_entry *pnode =
		list_entry(iter_impl->node, struct segment_entry, ln);

	segcol_iter_free(iter);
	
	/* create a list node containing the new segment */
	struct segment_entry *qnode;
	err = list_new_entry(&qnode, struct segment_entry, ln);
	if (err)
		return_error(err);

	qnode->segment = seg;

	/* 
	 * split the existing segment and insert the new segment
	 */
	
	/* where to split the existing segment */
	off_t split_index = offset - mapping;

	/* check if a split is actually needed or we just have to prepend 
	 * the new segment */
	if (split_index == 0) {
		list_insert_before(&pnode->ln, &qnode->ln);
	} else {
		segment_t *rseg;
		err = segment_split(pseg, &rseg, split_index);
		if (err)
			return_error(err);
		
		struct segment_entry *rnode;
		err = list_new_entry(&rnode, struct segment_entry, ln);
		if (err) {
			segment_merge(pseg, rseg);
			segment_free(rseg);
			return_error(err);
		}
		rnode->segment = rseg;

		list_insert_after(&pnode->ln, &qnode->ln);
		list_insert_after(&qnode->ln, &rnode->ln);
	}

	/* Set the cache at the inserted node */
	segcol_list_set_cache(impl, &qnode->ln, offset);

	return 0;
}

/*
 * The algorithm first deletes from the segcol list the entry chain that is
 * defined by first_entry and last_entry (the nodes that contain the start and
 * end of the range, respectively). This possibly deletes more than it should
 * because we completely delete the first and last segment, when perhaps only a
 * part of them should be deleted (F, L). We fix this later on by reinserting
 * a and b. Some of the comments below will refer to the following diagram:
 *
 *  P: first_entry_prev a: entry_a F: first_entry
 *  N: last_entry_next b: entry_b L: last_entry
 *  
 *  -[P]-[a|F]-[]-[]-[L|b]-[N]-  => -[]-[P]-[N]-[]- => -[P]-[a]-[b]-[N]-
 *          ^           ^
 *        offset  offset + length
 */
static int segcol_list_delete(segcol_t *segcol, segcol_t **deleted, off_t
		offset, off_t length)
{ 
	if (segcol == NULL || offset < 0 || length < 0)
		return_error(EINVAL);

	/* Check range for overflow */
	if (__MAX(off_t) - offset < length - 1 * (length != 0))
		return_error(EOVERFLOW);

	struct segcol_list_impl *impl = 
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	struct segment_entry *first_entry = NULL;
	struct segment_entry *last_entry = NULL;
	off_t first_mapping = -1;
	off_t last_mapping = -1;

	/* Find the first and last list nodes that contain the range */
	int err = find_seg_entry(segcol, &first_entry, &first_mapping, offset);
	if (err)
		return_error(err);

	/* 
	 * If the length is 0 return successfully without doing anything.
	 * This check is placed after the first find_seg_entry() so that the
	 * validity of offset (if it is in range) is checked first.
	 */
	if (length == 0)
		return 0;

	err = find_seg_entry(segcol, &last_entry, &last_mapping,
			offset + length - 1 * (length != 0));

	if (err)
		return_error(err);

	if (first_entry == NULL || last_entry == NULL 
		|| first_entry->segment == NULL || last_entry->segment == NULL)
		return_error(EINVAL);

	segcol_list_clear_cache(impl);

	/* 
	 * entry_a will hold the part of the first segment that we must
	 * put back into the segcol.
	 * entry_b will hold the part of the last segment that we must
	 * put back into the segcol.
	 */
	struct segment_entry *entry_a;
	struct segment_entry *entry_b;

	err = list_new_entry(&entry_a, struct segment_entry, ln);
	if (err)
		return_error(err);
	err = list_new_entry(&entry_b, struct segment_entry, ln);
	if (err) {
		free(entry_a);
		return_error(err);
	}

	/* 
	 * The nodes that should go before and after entry_a and entry_b, respectively 
	 * when and if we should insert them back in.
	 */
	struct segment_entry *entry_a_prev =
		list_entry(first_entry->ln.prev, struct segment_entry, ln);

	struct segment_entry *entry_b_next =
		list_entry(last_entry->ln.next, struct segment_entry, ln);

    /* Delete the chain */
	list_delete_chain(&first_entry->ln, &last_entry->ln);

	/* Calculate new size, after having deleted the chain */
	off_t new_size;
	segcol_get_size(segcol, &new_size);

	off_t last_seg_size;
	segment_get_size(last_entry->segment, &last_seg_size);

	new_size -= last_mapping + last_seg_size - first_mapping;

	/* 
	 * Handle first node: Split first segment so that the first node contains
	 * only segment F (and store the remaining in entry_a). We only have to do
	 * something if the range to delete starts after the beginning of the first
	 * segment.  Otherwise the whole segment should be deleted which has
	 * already been done by list_delete_chain.
	 */
	if (first_mapping < offset) {
		segment_t *tmp_seg;
		err = segment_split(first_entry->segment, &tmp_seg, offset - first_mapping);
		if (err)
			goto fail_first_entry;

		entry_a->segment = first_entry->segment;
		first_entry->segment = tmp_seg;
	} else {
		free(entry_a);
		entry_a = NULL;
	}

	/* 
	 * Handle last node: Split last segment so that the last node contains only
	 * segment L (and store the remaining in entry_b). We only have to do
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
		int last_seg_dec = 0;
		
		if (first_entry == last_entry)
			last_seg_dec = offset - first_mapping;

		segment_t *tmp_seg;
		err = segment_split(last_entry->segment, &tmp_seg,
				offset + length - last_mapping - last_seg_dec);
		if (err)
			goto fail_last_entry;

		entry_b->segment = tmp_seg;
	} else {
		free(entry_b);
		entry_b = NULL;
	}

	/* 
	 * Insert incorrectly deleted parts of the segments back into segcol
	 */

	if (entry_a != NULL)
		list_insert_after(&entry_a_prev->ln, &entry_a->ln);

	if (entry_b != NULL)
		list_insert_before(&entry_b_next->ln, &entry_b->ln);

	/* Create a new segcol_t and put in the deleted segments */
	segcol_t *deleted_tmp;
	err = segcol_list_new(&deleted_tmp);
	if (err)
		goto fail_segcol_new_deleted;

	struct segcol_list_impl *deleted_impl = 
		(struct segcol_list_impl *) segcol_get_impl(deleted_tmp);

	list_insert_chain_after(seg_list_head(deleted_impl->list), &first_entry->ln,
			&last_entry->ln);

	/* Either return the deleted segments or free them */
	if (deleted != NULL) 
		*deleted = deleted_tmp;
	else
		segcol_free(deleted_tmp);

	/* Set the cache at the node after the deleted range */
	if (entry_b != NULL)
		segcol_list_set_cache(impl, &entry_b->ln, offset);
	else if (entry_b_next->ln.next != &entry_b_next->ln)
		segcol_list_set_cache(impl, &entry_b_next->ln, offset);

	return 0;
/* 
 * Handle failures so that the segcol is in its expected state
 * after a failure and there are no memory leaks.
 */
fail_segcol_new_deleted:
	if (entry_b != NULL)
		list_delete_chain(&entry_b->ln, &entry_b->ln);

	if (entry_a != NULL)
		list_delete_chain(&entry_a->ln, &entry_a->ln);

	if (entry_b != NULL) {
		segment_merge(last_entry->segment, entry_b->segment);
		free(entry_b->segment);
	}
fail_last_entry:
	if (entry_a != NULL) {
		segment_merge(entry_a->segment, first_entry->segment);
		free(first_entry->segment);
		first_entry->segment = entry_a->segment; 
	}
fail_first_entry:
	list_insert_before(&entry_b_next->ln, &last_entry->ln);
	list_insert_after(&entry_a_prev->ln, &first_entry->ln);

	free(entry_a);
	free(entry_b);

	return_error(err);
}

static int segcol_list_find(segcol_t *segcol, segcol_iter_t **iter, off_t offset)
{
	if (segcol == NULL || iter == NULL || offset < 0)
		return_error(EINVAL);

	int err;

	/* Make sure offset is in range */
	off_t segcol_size;
	segcol_get_size(segcol, &segcol_size);

	if (offset >= segcol_size)
		return_error(EINVAL);

	struct segcol_list_impl *impl = 
		(struct segcol_list_impl *) segcol_get_impl(segcol);

	/* 
	 * Find the closest known node/mapping pair to the offset
	 * and start the search from there.
	 */
	struct list_node *cur_node = NULL;
	off_t cur_mapping = -1;

	segcol_list_get_closest_node(segcol, &cur_node, &cur_mapping, offset);
	
	int fix_mapping = 0;

	/* linear search of list nodes */
	while (cur_node != cur_node->next && cur_node != cur_node->prev) {
		struct segment_entry *snode =
			list_entry(cur_node, struct segment_entry, ln);

		segment_t *seg = snode->segment;
		off_t seg_size;
		err = segment_get_size(seg, &seg_size);

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
			 * Fix the mapping in the next iteration. Otherwise we would have
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
		return_error(err);
	
	struct segcol_list_iter_impl *iter_impl = 
		(struct segcol_list_iter_impl *) segcol_iter_get_impl(*iter);
	
	iter_impl->node = cur_node;
	iter_impl->mapping = cur_mapping;

	return 0;
}

static int segcol_list_iter_new(segcol_t *segcol, void **iter_impl)
{
	if (segcol == NULL || iter_impl == NULL)
		return_error(EINVAL);

	struct segcol_list_impl *impl =
		(struct segcol_list_impl *) segcol_get_impl(segcol);
	struct segcol_list_iter_impl **iter_impl1 =
		(struct segcol_list_iter_impl **)iter_impl;

	*iter_impl1 = malloc(sizeof(struct segcol_list_iter_impl));

	(*iter_impl1)->node = seg_list_head(impl->list)->next;
	(*iter_impl1)->mapping = 0;

	return 0;
}


static int segcol_list_iter_next(segcol_iter_t *iter)
{
	if (iter == NULL)
		return_error(EINVAL);
	
	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);

	if (iter_impl->node != iter_impl->node->next) {
		off_t node_size;
		
		struct segment_entry *snode =
			list_entry(iter_impl->node, struct segment_entry, ln);

		segment_get_size(snode->segment, &node_size);
		iter_impl->node = iter_impl->node->next;
		iter_impl->mapping = iter_impl->mapping + node_size;
	} 

	return 0;
}

static int segcol_list_iter_get_segment(segcol_iter_t *iter, segment_t **seg)
{
	if (iter == NULL || seg == NULL)
		return_error(EINVAL);

	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
	
	*seg = list_entry(iter_impl->node, struct segment_entry, ln)->segment;

	return 0;
}

static int segcol_list_iter_get_mapping(segcol_iter_t *iter, off_t *mapping)
{
	if (iter == NULL || mapping == NULL)
		return_error(EINVAL);

	struct segcol_list_iter_impl *iter_impl = segcol_iter_get_impl(iter);
	
	*mapping = iter_impl->mapping;

	return 0;
}

static int segcol_list_iter_is_valid(segcol_iter_t *iter, int *valid)
{
	if (iter == NULL || valid == NULL)
		return_error(EINVAL);

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
		return_error(EINVAL);

	free(segcol_iter_get_impl(iter));

	return 0;
}
