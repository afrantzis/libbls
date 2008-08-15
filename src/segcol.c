#include <stdlib.h>

#include "segcol.h"
#include "segcol_internal.h"

struct segcol {
	int (*free)(segcol_t *segcol);
	int (*insert)(segcol_t *segcol, off_t offset, segment_t *seg);
	segcol_t *(*delete)(segcol_t *segcol, off_t offset, size_t length);
	segcol_iter_t *(*find)(segcol_t *segcol, off_t offset);
	void *(*iter_new)(segcol_t *segcol);
	int (*iter_next)(segcol_iter_t *iter);
	segment_t *(*iter_get_segment)(segcol_iter_t *iter);
	off_t (*iter_get_mapping)(segcol_iter_t *iter);
	int (*iter_is_valid)(segcol_iter_t *iter);
	int (*iter_free)(segcol_iter_t *segcol);

	void *impl;
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
		int (*insert)(segcol_t *segcol, off_t offset, segment_t *seg),
		segcol_t *(*delete)(segcol_t *segcol, off_t offset, size_t length),
		segcol_iter_t *(*find)(segcol_t *segcol, off_t offset),
		void *(*iter_new)(segcol_t *segcol),
		int (*iter_next)(segcol_iter_t *iter),
		segment_t *(*iter_get_segment)(segcol_iter_t *iter),
		off_t (*iter_get_mapping)(segcol_iter_t *iter),
		int (*iter_is_valid)(segcol_iter_t *iter),
		int (*iter_free)(segcol_iter_t *segcol)
		)
{

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
 *	- list
 *
 * @param impl the implementation to use
 *
 * @return the created segcol_t or NULL on error
 */
segcol_t *segcol_new(char *impl)
{
	
	segcol_t *segcol = malloc(sizeof(segcol_t));

	if (!strncmp(impl, "list", 4))
		segcol_list_new(segcol);	

	return segcol;
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
 * @return a new segcol_t containing the deleted segments or NULL on error
 */
segcol_t *segcol_delete(segcol_t *segcol, off_t offset, size_t length)
{
	return (*segcol->delete)(segcol->impl, offset, length);
}

/**
 * Finds the segment that contains a given logical offset.
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
 * Gets a new (forward) iterator for a segcol_t.
 *
 * @param segcol the segcol_t
 *
 * @return a forward segcol_iter_t or NULL on error
 */
segcol_iter_t *segcol_iter_new(segcol_t *segcol)
{
	segcol_iter_t *iter = malloc(sizeof(segcol_iter_t));

	iter->segcol = segcol;
	iter->impl = (*segcol->iter_new)(segcol);

	return iter;
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
 * 
 * @return 1 if the segcol_iter_t is valid, 0 otherwise
 */
int segcol_iter_is_valid(segcol_iter_t *iter)
{
	return (*iter->segcol->iter_is_valid)(iter);
}

/**
 * Gets the segment pointed to by a segcol_iter_t.
 *
 * @param iter the iter to use
 *
 * @return the pointed segment or NULL if the iterator is invalid
 */
segment_t *segcol_iter_get_segment(segcol_iter_t *iter)
{
	return (*iter->segcol->iter_get_segment)(iter);
}

/**
 * Gets the mapping (logical offset) of the segment pointed to by a
 * segcol_iter_t.
 *
 * @param iter the iter to use
 *
 * @return the mapping of the pointed segment or -1 if the iterator is invalid
 */
off_t segcol_iter_get_mapping(segcol_iter_t *iter)
{
	return (*iter->segcol->iter_get_mapping)(iter);
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
