#include <stdlib.h>
#include <errno.h>

#include "segcol.h"
#include "segcol_internal.h"

struct segcol {
	void *impl;
	size_t size;
	
	int (*free)(segcol_t *segcol);
	int (*append)(segcol_t *segcol, segment_t *seg); 
	int (*insert)(segcol_t *segcol, off_t offset, segment_t *seg); 
	int (*delete)(segcol_t *segcol, segcol_t **deleted, off_t offset, size_t length);
	int (*find)(segcol_t *segcol, segcol_iter_t **iter, off_t offset);
	int (*iter_new)(segcol_t *segcol, void **iter);
	int (*iter_next)(segcol_iter_t *iter);
	int (*iter_is_valid)(segcol_iter_t *iter, int *valid);
	int (*iter_get_segment)(segcol_iter_t *iter, segment_t **seg);
	int (*iter_get_mapping)(segcol_iter_t *iter, off_t *mapping);
	int (*iter_free)(segcol_iter_t *iter);
};


struct segcol_iter {
	segcol_t *segcol;
	void *impl;
};


/**
 * Registers functions
 */
void segcol_register_impl(segcol_t *segcol,
		void *impl,
		int (*free)(segcol_t *segcol),
		int (*append)(segcol_t *segcol, segment_t *seg), 
		int (*insert)(segcol_t *segcol, off_t offset, segment_t *seg), 
		int (*delete)(segcol_t *segcol, segcol_t **deleted, off_t offset, size_t length),
		int (*find)(segcol_t *segcol, segcol_iter_t **iter, off_t offset),
		int (*iter_new)(segcol_t *segcol, void **iter),
		int (*iter_next)(segcol_iter_t *iter),
		int (*iter_is_valid)(segcol_iter_t *iter, int *valid),
		int (*iter_get_segment)(segcol_iter_t *iter, segment_t **seg),
		int (*iter_get_mapping)(segcol_iter_t *iter, off_t *mapping),
		int (*iter_free)(segcol_iter_t *iter)
		)
{
	segcol->impl = impl;
	segcol->free = free;
	segcol->append = append;
	segcol->insert = insert;
	segcol->delete = delete;
	segcol->find = find;
	segcol->iter_new = iter_new;
	segcol->iter_next = iter_next;
	segcol->iter_is_valid = iter_is_valid;
	segcol->iter_get_segment = iter_get_segment;
	segcol->iter_get_mapping = iter_get_mapping;
	segcol->iter_free = iter_free;
}

/**
 * Gets the implementation of a segcol_t
 */
void *segcol_get_impl(segcol_t *segcol)
{
	return segcol->impl;
}

/**
 * Gets the implementation of a segcol_t
 */
void *segcol_iter_get_impl(segcol_iter_t *iter)
{
	return iter->impl;
}

/**
 * Creates a new segcol_t.
 *
 * The new segcol_t is can be implemented in a number of ways. The impl
 * argument specifies the implementation to use. Available implementations
 * are:
 *	- "list"
 *
 * @param impl the implementation to use
 *
 * @param[out] segcol the created segcol_t
 *
 * @return the operation error code
 */
int segcol_new(segcol_t **segcol, char *impl)
{
	if (segcol == NULL || impl == NULL)
		return EINVAL;

	*segcol = malloc(sizeof(segcol_t));

	(*segcol)->size = 0;

	if (!strncmp(impl, "list", 4))
		segcol_list_new(*segcol);	
	else
		return EINVAL;

	return 0;
}

/**
 * Frees the resources of a segcol_t.
 *
 * After the invocation of this function it is an error to use the
 * freed segcol_t.
 *
 * @param segcol the segcol_t to free
 *
 * @return the operation error code
 */
int segcol_free(segcol_t *segcol)
{
	(*segcol->free)(segcol);
	free(segcol);
	return 0;
}

/**
 * Appends a segment to the segcol_t.
 * 
 * After the invocation of this function the segcol_t is responsible
 * for the memory handling of the specified segment. The segment should
 * not be further manipulated by the user.
 *
 * @param segcol the segcol_t to append to
 * @param seg the segment to append
 * 
 * @return the operation error code
 */
int segcol_append(segcol_t *segcol, segment_t *seg)
{
	int err = (*segcol->append)(segcol, seg);

	if (!err) {
		size_t size;
		segment_get_size(seg, &size);
		segcol->size += size;
	}

	return err;
}

/**
 * Inserts a segment into the segcol_t.
 * 
 * After the invocation of this function the segcol_t is responsible
 * for the memory handling of the specified segment. The segment should
 * not be further manipulated by the user.
 *
 * @param segcol the segcol_t to insert into
 * @param offset the logical offset at which to insert
 * @param seg the segment to insert
 * 
 * @return the operation error code
 */
int segcol_insert(segcol_t *segcol, off_t offset, segment_t *seg)
{
	int err = (*segcol->insert)(segcol, offset, seg);

	if (!err) {
		size_t size;
		segment_get_size(seg, &size);
		segcol->size += size;
	}

	return err;
}

/**
 * Deletes a logical range from the segcol_t.
 * 
 * @param segcol the segcol_t to delete from
 * @param[out] deleted a new segcol_t containing the deleted segments or NULL on error
 * @param offset the logical offset to start deleting at
 * @param length the length of the range to delete
 * 
 * @return the operation error code
 */
int segcol_delete(segcol_t *segcol, segcol_t **deleted, off_t offset, size_t length)
{
	int err = (*segcol->delete)(segcol, deleted, offset, length);

	if (!err) {
		segcol->size -= length;
	}
	
	return err;
}

/**
 * Finds the segment that contains a given logical offset.
 *
 * @param segcol the segcol_t to search in
 * @param[out] iter a segcol_iter_t pointing to the found segment
 * @param offset the offset to search for
 *
 * @return the operation error code
 */
int segcol_find(segcol_t *segcol, segcol_iter_t **iter, off_t offset)
{
	return (*segcol->find)(segcol, iter, offset);
}

/**
 * Gets a new (forward) iterator for a segcol_t.
 *
 * @param segcol the segcol_t
 * @param[out] iter a forward segcol_iter_t
 *
 * @return the operation error code
 */
int segcol_iter_new(segcol_t *segcol, segcol_iter_t **iter)
{
	*iter = malloc(sizeof(segcol_iter_t));

	(*iter)->segcol = segcol;
	(*segcol->iter_new)(segcol, &(*iter)->impl);

	return 0;
}

/**
 * Moves the segcol_iter_t to the next element.
 *
 * @param iter the segcol_iter_t to move
 *
 * @return the operation error code
 */
int segcol_iter_next(segcol_iter_t *iter)
{
	return (*iter->segcol->iter_next)(iter);
}

/**
 * Whether the iter points to a valid element.
 *
 * @param iter the segcol_iter_t to check
 * @param[out] valid 1 if the segcol_iter_t is valid, 0 otherwise
 *
 * @return the operation error code
 */
int segcol_iter_is_valid(segcol_iter_t *iter, int *valid)
{
	return (*iter->segcol->iter_is_valid)(iter, valid);
}

/**
 * Gets the segment pointed to by a segcol_iter_t.
 *
 * @param iter the iter to use
 * @param[out] the pointed segment or NULL if the iterator is invalid
 *
 * @return the operation error code
 */
int segcol_iter_get_segment(segcol_iter_t *iter, segment_t **seg)
{
	return (*iter->segcol->iter_get_segment)(iter, seg);
}

/**
 * Gets the mapping (logical offset) of the segment pointed to by a
 * segcol_iter_t.
 *
 * @param iter the iter to use
 * @param[out] mapping the mapping of the pointed segment or -1 if the iterator is invalid
 *
 * @return the operation error code
 */
int segcol_iter_get_mapping(segcol_iter_t *iter, off_t *mapping)
{
	return (*iter->segcol->iter_get_mapping)(iter, mapping);
}

/**
 * Frees a segcol_iter_t.
 * 
 * It is an error to use a segcol_iter_t after freeing it.
 *
 * @return the operation error code
 */
int segcol_iter_free(segcol_iter_t *iter)
{
	(*iter->segcol->iter_free)(iter);
	free(iter);
}

int segcol_get_size(segcol_t *segcol, size_t *size)
{
	if (segcol == NULL || size == NULL)
		return EINVAL;

	*size = segcol->size;

	return 0;
}
