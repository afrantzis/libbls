#include "segcol.h"
#include "segcol_internal.h"

/**
 * Creates a new segcol_t.
 *
 * The new segcol_t is can be implemented in a number of ways. The impl
 * argument specifies the implementation to use. Available implementations
 * are:
 *	- list
 *
 * @param impl the implementation to use
 *
 * @return the created segcol_t
 */
segcol_t *segcol_new(char *impl)
{
	if (!strncmp(impl, "list", 4))
		return segcol_list_new();	
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
	(*segcol->free)(segcol->impl);
	free(segcol);
	return 0;
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
	return (*segcol->insert)(segcol->impl, offset, seg);
}

/**
 * Deletes a logical range from the segcol_t.
 * 
 * @param segcol the segcol_t to delete from
 * @param offset the logical offset to start deleting at
 * @param length the length of the range to delete
 * 
 * @return a new segcol_t containing the deleted segments
 */
segcol_t *segcol_delete(segcol_t *segcol, off_t offset, size_t length)
{
	return (*segcol->delete)(segcol->impl, offset, length);
}

/**
 * Finds the segment that contains a given logical offset
 *
 * @param segcol the segcol_t to search in
 * @param offset the offset to search for
 *
 * @return a segcol_iter_t pointing to the found segment
 */
segcol_iter_t *segcol_find(segcol_t *segcol, off_t offset)
{
	return (*segcol->find)(segcol->impl, offset);
}

/**
 * Moves the segcol_iter_t to the next element
 *
 * @param iter the segcol_iter_t to move
 *
 * @return the operation error code
 */
int segcol_iter_next(segcol_iter *iter)
{
	return (*segcol->iter_next)(iter);
}

/**
 * Whether the iter points to a valid element.
 *
 * @param iter the segcol_iter_t to check
 * 
 * @return 1 if the segcol_iter_t is valid, 0 otherwise
 */
int segcol_iter_is_valid(segcol_iter_t *iter)
{
	return (*segcol->iter_is_valid)(iter);
}

segment_t *segcol_iter_get_segment(segcol_iter *iter)
{
	return (*segcol->iter_get_segment)(iter);
}

off_t *segcol_iter_get_mapping(segcol_iter *iter)
{
	return (*segcol->iter_get_mapping)(iter);
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
	return (*segcol->iter_free)(iter);
}
