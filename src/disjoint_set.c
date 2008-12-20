/**
 * @file disjoint_set.c
 *
 * Disjoint-set implementation
 */
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include "disjoint_set.h"

struct disjoint_set {
	size_t *parent;
	size_t *rank;
	size_t size;
};

/********************/
/* Helper functions */
/********************/

/**
 * Links two sets.
 * 
 * @param ds the disjoint_set_t the sets belong to
 * @param set1 the id (root) of set1 
 * @param set2 the id (root) of set2
 */
static void link_sets(disjoint_set_t *ds, size_t set1, size_t set2)
{
	size_t rank1 = ds->rank[set1];
	size_t rank2 = ds->rank[set2];

	if (rank1 > rank2)
		ds->parent[set2] = set1;
	else {
		ds->parent[set1] = set2;
		if (rank1 == rank2)
			ds->rank[set2] += 1;
	}
}

/** 
 * Finds the set containing an element.
 * 
 * @param ds the disjoint_set_t to search in
 * @param id the id of the element to find the set of 
 * 
 * @return the id (root) of the found set 
 */
static size_t find_set(disjoint_set_t *ds, size_t id)
{
	if (id != ds->parent[id])
		ds->parent[id] = find_set(ds, ds->parent[id]);

	return ds->parent[id];
}

/*****************/
/* API functions */
/*****************/

/** 
 * Creates a new disjoint-set.
 *
 * The disjoint-set contains integer elements
 * from 0 to size - 1, initially each in its own set.
 * 
 * @param[out] ds the created disjoint_set_t
 * @param size the size of the disjoint-set 
 * 
 * @return the operation error code 
 */
int disjoint_set_new(disjoint_set_t **ds, size_t size)
{
	if (ds == NULL)
		return EINVAL;

	disjoint_set_t *p = malloc(sizeof *p);
	if (p == NULL)
		return ENOMEM;

	p->parent = malloc(size * sizeof *p->parent);
	if (p->parent == NULL) {
		free(p);
		return ENOMEM;
	}

	p->rank = calloc(size, sizeof *p->rank);
	if (p->rank == NULL) {
		free(p->parent);
		free(p);
		return ENOMEM;
	}

	p->size = size;

	size_t i;
	for(i = 0; i < size; i++)
		p->parent[i] = i;

	*ds = p;

	return 0;
}

/** 
 * Frees a disjoint-set.
 * 
 * @param ds the disjoint_set_t to free 
 * 
 * @return the operation error code
 */
int disjoint_set_free(disjoint_set_t *ds)
{
	if (ds == NULL)
		return EINVAL;
	
	free(ds->parent);
	free(ds->rank);

	free(ds);

	return 0;
}

/** 
 * Unites the sets that contain two elements.
 * 
 * @param ds the disjoint_set_t that contains the elements
 * @param elem1 the first element 
 * @param elem2 the second element 
 * 
 * @return the operation error code
 */
int disjoint_set_union(disjoint_set_t *ds, size_t elem1, size_t elem2)
{
	if (ds == NULL || elem1 >= ds->size || elem2 >= ds->size)
		return EINVAL;

	size_t set1;
	size_t set2;

	disjoint_set_find(ds, &set1, elem1);
	disjoint_set_find(ds, &set2, elem2);

	link_sets(ds, set1, set2);

	return 0;
}

/** 
 * Finds the set containing an element.
 * 
 * @param ds the disjoint_set_t to search in 
 * @param[out] set the found set
 * @param elem the element to find the set of 
 * 
 * @return the operation error code
 */
int disjoint_set_find(disjoint_set_t *ds, size_t *set, size_t elem)
{
	if (ds == NULL || set == NULL || elem >= ds->size)
		return EINVAL;

	*set = find_set(ds, elem);

	return 0;

}
