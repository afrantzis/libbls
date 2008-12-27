#include <stdlib.h>
#include <errno.h>

#include "segcol.h"
#include "segcol_internal.h"
#include "type_limits.h"

struct segcol {
	void *impl;
	struct segcol_funcs *funcs; 

	off_t size;
};


struct segcol_iter {
	segcol_t *segcol;
	void *impl;
};


/**********************
 * Internal functions *
 **********************/

/**
 * Creates a segcol_t using a specific implementation.
 *
 * @param[out] segcol the created segcol_t
 * @param impl the implementation private data
 * @param funcs function pointers to the implementations' functions
 *
 * @return the operation status code
 */
int segcol_create_impl(segcol_t **segcol, void *impl,
		struct segcol_funcs *funcs)
{
	if (segcol == NULL)
		return EINVAL;

	*segcol = malloc(sizeof(segcol_t));

	if (*segcol == NULL)
		return ENOMEM;

	(*segcol)->size = 0;
	(*segcol)->impl = impl;
	(*segcol)->funcs = funcs;

	return 0;
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

/*****************
 * API functions *
 *****************/

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
	(*segcol->funcs->free)(segcol);
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
	off_t seg_size;
	segment_get_size(seg, &seg_size);

	/* Check for segcol->size overflow */
	if (__MAX(off_t) - segcol->size < seg_size)
		return EOVERFLOW;

	int err = (*segcol->funcs->append)(segcol, seg);

	if (!err) 
		segcol->size += seg_size;

	return err;
}

/**
 * Inserts a segment into the segcol_t.
 * 
 * After the invocation of this function the segcol_t is responsible
 * for the memory handling of the specified segment. The segment should
 * not be further manipulated by the caller.
 *
 * @param segcol the segcol_t to insert into
 * @param offset the logical offset at which to insert
 * @param seg the segment to insert
 * 
 * @return the operation error code
 */
int segcol_insert(segcol_t *segcol, off_t offset, segment_t *seg)
{
	off_t seg_size;
	segment_get_size(seg, &seg_size);

	/* Check for segcol->size overflow */
	if (__MAX(off_t) - segcol->size < seg_size)
		return EOVERFLOW;

	int err = (*segcol->funcs->insert)(segcol, offset, seg);

	if (!err) 
		segcol->size += seg_size;

	return err;
}

/**
 * Deletes a logical range from the segcol_t.
 *
 * @param segcol the segcol_t to delete from
 * @param[out] deleted if it is not NULL, on return it will point to a new 
 *                     segcol_t containing the deleted segments
 * @param offset the logical offset to start deleting at
 * @param length the length of the range to delete
 * 
 * @return the operation error code
 */
int segcol_delete(segcol_t *segcol, segcol_t **deleted, off_t offset, off_t length)
{
	int err = (*segcol->funcs->delete)(segcol, deleted, offset, length);

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
	return (*segcol->funcs->find)(segcol, iter, offset);
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
	(*segcol->funcs->iter_new)(segcol, &(*iter)->impl);

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
	return (*iter->segcol->funcs->iter_next)(iter);
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
	return (*iter->segcol->funcs->iter_is_valid)(iter, valid);
}

/**
 * Gets the segment pointed to by a segcol_iter_t.
 *
 * @param iter the iter to use
 * @param[out] seg the pointed segment or NULL if the iterator is invalid
 *
 * @return the operation error code
 */
int segcol_iter_get_segment(segcol_iter_t *iter, segment_t **seg)
{
	return (*iter->segcol->funcs->iter_get_segment)(iter, seg);
}

/**
 * Gets the mapping (logical offset) of the segment pointed to by a
 * segcol_iter_t.
 *
 * @param iter the iter to use
 * @param[out] mapping the mapping of the pointed segment or -1 if the 
 *             iterator is invalid
 *
 * @return the operation error code
 */
int segcol_iter_get_mapping(segcol_iter_t *iter, off_t *mapping)
{
	return (*iter->segcol->funcs->iter_get_mapping)(iter, mapping);
}

/**
 * Frees a segcol_iter_t.
 * 
 * It is an error to use a segcol_iter_t after freeing it.
 *
 * @param iter the segcol_iter_t to free
 *
 * @return the operation error code
 */
int segcol_iter_free(segcol_iter_t *iter)
{
	(*iter->segcol->funcs->iter_free)(iter);
	free(iter);

	return 0;
}

/**
 * Gets the size of the data contained in a segcol_t.
 * 
 * @param segcol the segcol to get the size of
 * @param[out] size the size of the segcol in bytes
 *
 * @return the operation error code
 */
int segcol_get_size(segcol_t *segcol, off_t *size)
{
	if (segcol == NULL || size == NULL)
		return EINVAL;

	*size = segcol->size;

	return 0;
}
