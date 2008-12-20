/**
 * @file disjoint_set.h
 *
 * Disjoint-set API
 */
#ifndef _BLESS_DISJOINT_SET_H
#define _BLESS_DISJOINT_SET_H

#include <sys/types.h>

typedef struct disjoint_set disjoint_set_t;

int disjoint_set_new(disjoint_set_t **ds, size_t size);

int disjoint_set_free(disjoint_set_t *ds);

int disjoint_set_union(disjoint_set_t *ds, size_t id1, size_t id2);

int disjoint_set_find(disjoint_set_t *ds, size_t *set_id, size_t id);

#endif 
