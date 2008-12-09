/**
 * @file overlap_graph.c 
 *
 * Overlap graph implementation
 */
#include "overlap_graph.h"
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

struct edge {
	off_t weight;
	int vertex_id;
	struct edge *next;
};

struct vertex {
	segment_t *segment;
	off_t mapping;
	struct edge *head;
};

struct overlap_graph {
	struct vertex *vertices; 
	size_t capacity;
	size_t size;
	struct edge tail;
};

/********************/
/* Helper functions */
/********************/


/** 
 * Calculates the overlap of two ranges.
 * 
 * @param start1 start of first range
 * @param size1 size of first range 
 * @param start2 start of second range
 * @param size2 size of second range 
 * 
 * @return the operation error code 
 */
static off_t calculate_overlap(off_t start1, off_t size1, off_t start2,
		off_t size2)
{
	if (size1 == 0 || size2 == 0)
		return 0;

	off_t end1 = start1 + size1 - 1;
	off_t end2 = start2 + size2 - 1;

	off_t overlap = 0;

	if (start2 >= start1 && start2 <= end1)
		overlap = end1 - start2 + 1;

	if (end2 >= start1 && end2 <= end1) {
		if (overlap == 0)
			overlap = end2 - start1 + 1;
		else
			overlap -= end2 - end1;
	}

	return overlap;
}

/** 
 * Adds an edge to the overlap graph. 
 * 
 * @param g the overlap graph 
 * @param src_id the id of the source of the edge
 * @param dst_id the id of the destination of the edges 
 * @param weight the weight of the edge
 * 
 * @return the operation error code
 */
static int overlap_graph_add_edge(overlap_graph_t *g, size_t src_id, size_t dst_id,
		off_t weight)
{
	/* Search src edges for dst */
	struct edge *e = g->vertices[src_id].head;
	while (e->next != &g->tail) {
		if (e->next->vertex_id == dst_id)
			break;
		e = e->next;
	}

	/* if we didn't find edge add it */
	if (e->next == &g->tail) {
		struct edge *edst = malloc (sizeof *edst);
		if (edst == NULL)
			return ENOMEM;

		edst->vertex_id = dst_id;

		edst->next = e->next;
		e->next = edst;
	}

	e->next->weight = weight;

	return 0;
}

/*****************/
/* API functions */
/*****************/

/**
 * Creates an new overlap graph.
 *
 * @param[out] g the created overlap graph
 * @param capacity the initial capacity of the graph (number of nodes)
 * 
 * @return the operation error code 
 */
int overlap_graph_new(overlap_graph_t **g, size_t capacity)
{
	if (g == NULL)
		return EINVAL;

	overlap_graph_t *p;

	p = malloc (sizeof *p);
	if (p == NULL)
		return ENOMEM;

	p->vertices = malloc(capacity * sizeof *p->vertices);
	if (p->vertices == NULL) {
		free (p);
		return ENOMEM;
	}

	p->capacity = capacity;
	p->size = 0;
	p->tail.next = &p->tail;

	*g = p;

	return 0;
}

/**
 * Frees an overlap graph.
 *
 * This function does not free the segments
 * associated with the overlap graph.
 *
 * @param g the overlap graph to free
 *
 * @return the operation error code
 */
int overlap_graph_free(overlap_graph_t *g)
{
	/* For every vertex... */
	int i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];
		
		/* ...Free its edges */
		struct edge *e = v->head;
		while (e != &g->tail) {
			struct edge *next = e->next;
			free(e);
			e = next;
		}
	}

	free(g->vertices);

	return 0;
}

/** 
 * Adds a segment to an overlap graph.
 * 
 * @param g the overlap graph to add the segment to 
 * @param seg the segment to add
 * @param mapping the mapping of the segment in its buffer
 * 
 * @return the operation error code 
 */
int overlap_graph_add_segment(overlap_graph_t *g, segment_t *seg,
		off_t mapping)
{
	/* Check if we have enough memory */
	if (g->size >= g->capacity) {
		size_t new_capacity = ((5 * g->capacity) / 4) + 1;
		struct vertex *t = realloc(g->vertices, new_capacity * sizeof *t);
		if (t == NULL)
			return ENOMEM;

		g->vertices = t;
		g->capacity = new_capacity;
	}
	
	/* Add the new segment */
	struct vertex *v = &g->vertices[g->size++];

	v->segment = seg;
	v->mapping = mapping;
	v->head = malloc(sizeof *v->head);
	if (v->head == NULL)
		return ENOMEM;
	v->head->next = &g->tail;

	off_t seg_size;
	segment_get_size(seg, &seg_size);
	off_t seg_start;
	segment_get_start(seg, &seg_start);

	/* 
	 * Find all segments in the graph (including the new segment) that 
	 * "overlap" with the new segment and add edges to them.
	 */
	int i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];
		off_t vstart;
		segment_get_start(v->segment, &vstart);
		off_t vsize;
		segment_get_size(v->segment, &vsize);

		
		/* if v->segment in the file overlaps with seg in the buffer */
		off_t overlap1 = calculate_overlap(vstart, vsize, mapping, seg_size);
		/* if seg in the file overlaps with v->segment in the buffer */
		off_t overlap2 = calculate_overlap(seg_start, seg_size, v->mapping,
				vsize);
		
		/* Add the overlap edges */
		if (overlap1 != 0)
			overlap_graph_add_edge(g, i, g->size - 1, overlap1);
		if (overlap2 != 0)
			overlap_graph_add_edge(g, g->size - 1, i, overlap2);
	}


	return 0;
}


/**
 * Exports an overlap graph to the dot format.
 *
 * @param g the overlap graph to export
 * @param fd the file descriptor to write the data to
 *
 * @return the operation error code
 */
int overlap_graph_export_dot(overlap_graph_t *g, int fd)
{
	if (g == NULL)
		return EINVAL;

	FILE *fp = fdopen(fd, "w");
	if (fp == NULL)
		return EINVAL;

	/* Write dot header */
	fprintf(fp, "digraph overlap_graph {\n");

	/* For every vertex... */ 
	int i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];

		/* ...add all its outgoing edges */
		struct edge *e = v->head->next;
		while (e != &g->tail) {
			fprintf(fp, "%d -> %d [label = %d]\n", i, e->vertex_id, e->weight);
			e = e->next;
		}
	}

	fprintf(fp, "}\n");
	fflush(fp);

	return 0;
}
