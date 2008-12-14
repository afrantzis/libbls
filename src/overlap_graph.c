/**
 * @file overlap_graph.c 
 *
 * Overlap graph implementation
 */
#include "overlap_graph.h"
#include "disjoint_set.h"
#include "priority_queue.h"

#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/**
 * An overlap graph edge
 */
struct edge {
	off_t weight; /**< the weight of the edge */
	size_t src_id; /**< the source vertex of the edge */
	size_t dst_id; /**< the destination vertex of the edge */
	int in_spanning_tree; /**< whether the edge is part of the spanning tree */
	struct edge *next; /**< the next edge in the list */
};

/**
 * An overlap graph vertex.
 */
struct vertex {
	segment_t *segment; /**< the segment the vertex represents */
	off_t mapping; /**< the mapping of the segment the vertex represents */
	off_t self_loop_weight; /**< the weight of the self-loop (0: no loop) */
	size_t n_incoming_edges; /**< the number of incoming edges except self-loop */
	struct edge *head; /**< the head of the linked list holding the outgoing
						 edges of the vertex. */
};

/**
 * An overlap graph.
 */
struct overlap_graph {
	struct vertex *vertices; /**< the vertices of the graph */
	size_t capacity; /**< the vertex capacity of the graph */
	size_t size; /**< the actual number of vertices in the graph */
	struct edge tail; /**< a tail edge used in edge lists */
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
			overlap -= end1 - end2;
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
		if (e->next->dst_id == dst_id)
			break;
		e = e->next;
	}

	/* if we didn't find an edge from src to dst, add it */
	if (e->next == &g->tail) {
		struct edge *edst = malloc (sizeof *edst);
		if (edst == NULL)
			return ENOMEM;

		g->vertices[dst_id].n_incoming_edges += 1;

		edst->src_id = src_id;
		edst->dst_id = dst_id;
		edst->in_spanning_tree = 0;

		edst->next = e->next;
		e->next = edst;
	}

	/* Update weight */
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
 * @param capacity the initial capacity (number of vertices) of the graph
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
	v->self_loop_weight = 0;
	v->n_incoming_edges = 0;
	v->head = malloc(sizeof *v->head);
	if (v->head == NULL)
		return ENOMEM;
	v->head->next = &g->tail;

	off_t seg_size;
	segment_get_size(seg, &seg_size);
	off_t seg_start;
	segment_get_start(seg, &seg_start);

	/* 
	 * Find all segments in the graph (excluding the new segment) that 
	 * "overlap" with the new segment and add edges to them. We handle 
	 * self-overlaps separately later because they are marked differently
	 * in the graph (as information at the node, not as a separate edge).
	 */
	int i;
	for (i = 0; i < g->size - 1; i++) {
		struct vertex *vtmp = &g->vertices[i];
		off_t vstart;
		segment_get_start(vtmp->segment, &vstart);
		off_t vsize;
		segment_get_size(vtmp->segment, &vsize);

		/* if v->segment in the file overlaps with seg in the buffer */
		off_t overlap1 = calculate_overlap(vstart, vsize, mapping, seg_size);
		/* if seg in the file overlaps with v->segment in the buffer */
		off_t overlap2 = calculate_overlap(seg_start, seg_size, vtmp->mapping,
				vsize);

		/* Add the overlap edges */
		if (overlap1 != 0)
			overlap_graph_add_edge(g, i, g->size - 1, overlap1);
		if (overlap2 != 0)
			overlap_graph_add_edge(g, g->size - 1, i, overlap2);
	}

	/* Check if segment overlaps with itself */
	v->self_loop_weight = calculate_overlap(seg_start, seg_size, mapping,
			seg_size);

	return 0;
}


/**
 * Finds the maximum spanning tree of a graph.
 *
 * This algorithm doesn't change the graph apart from
 * marking the edges as included or not in the spanning tree.
 *
 * @param g the graph to find the maximum spanning tree of
 *
 * @return the operation error code
 */
int overlap_graph_max_spanning_tree(overlap_graph_t *g)
{
	/* 
	 * Create a disjoint-set to hold the vertices of the graph. This is used
	 * below to efficiently check whether two nodes are connected. Initially
	 * all nodes are disconnected.
	 */
	disjoint_set_t *ds;
	int err = disjoint_set_new(&ds, g->size);
	if (err)
		return err;

	/* Create a max priority queue and add all the edges. */
	priority_queue_t *pq;
	err = priority_queue_new(&pq, g->size);
	if (err)
		return err;

	/* For every vertex... */ 
	int i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];
		/* ...reset the number of incoming edges */
		v->n_incoming_edges = 0;
		/* ...add all its outgoing edges to the priority queue */
		struct edge *e = v->head->next;
		while (e != &g->tail) {
			/* mark all edges as not in spanning tree */
			e->in_spanning_tree = 0;
			err = priority_queue_add(pq, e, e->weight, NULL);
			if (err)
				return err;
			e = e->next;
		}
	}

	/* 
	 * Process the edges in non-increasing order of weight using the priority 
	 * queue (remember this a *maximum* spanning tree algorithm).
	 */
	size_t pq_size;
	while (!priority_queue_get_size(pq, &pq_size) && pq_size > 0) {
		struct edge *e;
		err = priority_queue_remove_max(pq, (void **)&e);
		if (err)
			return err;

		/* Check if the vertexes are already connected through a path */
		size_t set1;
		size_t set2;
		err = disjoint_set_find(ds, &set1, e->src_id);
		if (err)
			return err;
		err = disjoint_set_find(ds, &set2, e->dst_id);
		if (err)
			return err;

		/* If they are not connected, add the edge */
		if (set1 != set2) {
			/* Mark the edge as used in the spanning tree */
			e->in_spanning_tree = 1;
			/* Mark the nodes as connected */
			err = disjoint_set_union(ds, e->src_id, e->dst_id);
			if (err)
				return err;
			/* Increase the number of incoming edges of the destination */
			g->vertices[e->dst_id].n_incoming_edges += 1;

		}
	}

	priority_queue_free(pq);

	disjoint_set_free(ds);

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

		/* ...print it along with number of incoming edges */
		fprintf(fp, "%d [label = %d]\n", i, v->n_incoming_edges);

		/* ...print its self-loop if it has one */
		if (v->self_loop_weight != 0) {
			fprintf(fp, "%d -> %d [label = %jd]\n", i, i,
					(intmax_t)v->self_loop_weight);
		}

		/* 
		 * ...print all its outgoing edges along with their weight (bold if
		 * they are in the spanning tree)
		 */
		struct edge *e = v->head->next;
		while (e != &g->tail) {
			fprintf(fp, "%d -> %d [label = %jd%s]\n", e->src_id, e->dst_id,
					(intmax_t)e->weight,
					e->in_spanning_tree == 1 ? " style = bold" : "");
			e = e->next;
		}
	}

	fprintf(fp, "}\n");
	fflush(fp);

	return 0;
}
