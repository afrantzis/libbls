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
 * @file overlap_graph.c 
 *
 * Overlap graph implementation
 */
#include "overlap_graph.h"
#include "disjoint_set.h"
#include "priority_queue.h"
#include "list.h"
#include "util.h"

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
	int removed; /**< whether the edge has been removed from the graph */
	struct edge *next; /**< the next edge in the list */
};

/**
 * An overlap graph vertex.
 */
struct vertex {
	segment_t *segment; /**< the segment the vertex represents */
	off_t mapping; /**< the mapping of the segment the vertex represents */
	off_t self_loop_weight; /**< the weight of the self-loop (0: no loop) */
	size_t in_degree; /**< the number of incoming edges except self-loop */
	size_t out_degree; /**< the number of outgoing edges except self-loop */
	int visited; /**< Marker used during graph traversals */
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

	if (start2 < start1 && end2 > end1)
		overlap = size1;

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
	g->vertices[src_id].out_degree += 1;

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
			return_error(ENOMEM);

		g->vertices[dst_id].in_degree += 1;

		edst->src_id = src_id;
		edst->dst_id = dst_id;
		edst->removed = 0;

		edst->next = e->next;
		e->next = edst;
	}

	/* Update weight */
	e->next->weight = weight;

	return 0;
}

/** 
 * Visit depth-first the vertices of an overlap graph prepending
 * them to the list as they finish.
 *
 * @param g the overlap graph
 * @param n the vertex id of the vertex to visit
 * @param list the list to add the vertices
 *
 * @return the operation error code
 */
static int topo_visit(overlap_graph_t *g, size_t n, struct list *list)
{
	int err;

	struct vertex *v = &g->vertices[n];

	/* Mark the node as visited */
	v->visited = 1;

	/* Visit all adjacent vertices in a depth-first fashion */
	struct edge *e = v->head->next;
	while (e != &g->tail) {
		struct vertex *vtmp = &g->vertices[e->dst_id];
		if (e->removed == 0 && vtmp->visited == 0) {
			err = topo_visit(g, e->dst_id, list);
			if (err)
				return_error(err);
		}
			
		e = e->next;
	}

	/* Create a vertex entry */
	struct vertex_entry *entry;
	err = list_new_entry(&entry, struct vertex_entry, ln);
	if (err)
		return_error(err);

	/* Fill in entry */
	entry->segment = v->segment;
	entry->mapping = v->mapping;
	entry->self_loop_weight = v->self_loop_weight;

	/* Prepend the entry to the list */
	struct list_node *head = 
		list_head(list, struct vertex_entry, ln);

	list_insert_after(head, &entry->ln);

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
		return_error(EINVAL);

	overlap_graph_t *p;

	p = malloc (sizeof *p);
	if (p == NULL)
		return_error(ENOMEM);

	p->vertices = malloc(capacity * sizeof *p->vertices);
	if (p->vertices == NULL) {
		free (p);
		return_error(ENOMEM);
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
	size_t i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];
		segment_free(v->segment);

		/* ...Free its edges */
		struct edge *e = v->head;
		while (e != &g->tail) {
			struct edge *next = e->next;
			free(e);
			e = next;
		}
	}

	free(g->vertices);
	free(g);

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
	if (g == NULL || seg == NULL || mapping < 0)
		return_error(EINVAL);

	/* Check if we have enough memory */
	if (g->size >= g->capacity) {
		size_t new_capacity = ((5 * g->capacity) / 4) + 1;
		struct vertex *t = realloc(g->vertices, new_capacity * sizeof *t);
		if (t == NULL)
			return_error(ENOMEM);

		g->vertices = t;
		g->capacity = new_capacity;
	}

	/* Add the new segment */
	struct vertex *v = &g->vertices[g->size++];

	int err = segment_copy(seg, &v->segment);
	if (err)
		return_error(err);

	v->mapping = mapping;
	v->self_loop_weight = 0;
	v->in_degree = 0;
	v->out_degree = 0;
	v->visited = 0;
	v->head = malloc(sizeof *v->head);
	if (v->head == NULL) {
		segment_free(v->segment);
		return_error(ENOMEM);
	}
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
	size_t i;
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
 * Removes cycles from the graph.
 *
 * This algorithm doesn't change the graph apart from
 * marking the edges as included or not in the graph.
 *
 * @param g the graph to remove the cycles of 
 *
 * @return the operation error code
 */
int overlap_graph_remove_cycles(overlap_graph_t *g)
{
	if (g == NULL)
		return_error(EINVAL);

	/* 
	 * Create a disjoint-set to hold the vertices of the graph. This is used
	 * below to efficiently check whether two nodes are connected. Initially
	 * all nodes are disconnected.
	 */
	disjoint_set_t *ds;
	int err = disjoint_set_new(&ds, g->size);
	if (err)
		return_error(err);

	/* Create a max priority queue and add all the edges. */
	priority_queue_t *pq;
	err = priority_queue_new(&pq, g->size);
	if (err) {
		disjoint_set_free(ds);
		return_error(err);
	}

	/* For every vertex... */ 
	size_t i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];
		/* Reset degree values */
		v->in_degree = 0;
		v->out_degree = 0;
		/* ...add all its outgoing edges to the priority queue */
		struct edge *e = v->head->next;
		while (e != &g->tail) {
			/* mark all edges as not included in the graph */
			e->removed = 1;
			err = priority_queue_add(pq, e, e->weight, NULL);
			if (err)
				goto out;
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
			goto out;

		size_t set1;
		size_t set2;
		err = disjoint_set_find(ds, &set1, e->src_id);
		if (err)
			goto out;
		err = disjoint_set_find(ds, &set2, e->dst_id);
		if (err)
			goto out;

		/* 
		 * If the edge cannot form a *directed cycle* add it. At this point we
		 * deviate from the classic Kruskal's algorithm. Kruskal's algorithm
		 * removes edges if they form undirected cycles but this is too
		 * aggresive for us because we only want to remove directed cycles. So
		 * we add some extra rules to keep some of the edges (but unfortunately
		 * not all) that would be unnecessarily removed by the classic
		 * algorithm. Note that the problem we are trying to solve (feedback
		 * arc set) is NP-Hard and this simple algorithm gives a reasonable
		 * approximation of the solution.
		 * 
		 * We add an edge when either:
		 * 1. The vertices are not previously connected (undirected cycle rule)
		 * 2. They are connected but the out-degree of the destination is 0
		 *    (if the destination has no outgoing edges it can not be part of
		 *    a directed cycle)
		 * 3. They are connected but the in-degree of the source is 0
		 *    (if the source has no incoming edges it can not be part of
		 *    a directed cycle)
		 */
		if (set1 != set2 || g->vertices[e->dst_id].out_degree == 0
				|| g->vertices[e->src_id].in_degree == 0) {
			/* Mark the edge as used in the graph */
			e->removed = 0;
			/* Mark the nodes as connected */
			err = disjoint_set_union(ds, e->src_id, e->dst_id);
			if (err)
				goto out;
			/* Increase the number of incoming edges of the destination */
			g->vertices[e->dst_id].in_degree += 1;
			/* Increase the number of outgoing edges of the source */
			g->vertices[e->src_id].out_degree += 1;

		}
	}

	priority_queue_free(pq);
	disjoint_set_free(ds);

	return 0;

out:
	priority_queue_free(pq);

	disjoint_set_free(ds);

	return_error(err);
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
		return_error(EINVAL);

	FILE *fp = fdopen(fd, "w");
	if (fp == NULL)
		return_error(EINVAL);

	/* Write dot header */
	fprintf(fp, "digraph overlap_graph {\n");

	/* For every vertex... */ 
	size_t i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];

		/* ...print it along with number of incoming/outgoing edges */
		fprintf(fp, "%zu [label = \"%zu-%zu/%zu\"]\n", i, i, v->in_degree,
				v->out_degree);

		/* ...print its self-loop if it has one */
		if (v->self_loop_weight != 0) {
			fprintf(fp, "%zu -> %zu [label = %jd]\n", i, i,
					(intmax_t)v->self_loop_weight);
		}

		/* 
		 * ...print all its outgoing edges along with their weight (dotted if
		 * they are not included in the graph)
		 */
		struct edge *e = v->head->next;
		while (e != &g->tail) {
			fprintf(fp, "%zu -> %zu [label = %jd%s]\n", e->src_id, e->dst_id,
					(intmax_t)e->weight,
					e->removed == 1 ? " style = dotted" : "");
			e = e->next;
		}
	}

	fprintf(fp, "}\n");
	fflush(fp);

	return 0;
}


/**
 * Gets the edges removed from the graph.
 *
 * @param g the overlap graph containing removed the edges
 * @param[out] edges the list containing entries of type struct edge_entry
 *
 * @return the operation error code
 */
int overlap_graph_get_removed_edges(overlap_graph_t *g, struct list **edges)
{
	if (g == NULL || edges == NULL)
		return_error(EINVAL);

	int err = list_new(edges, struct edge_entry, ln);
	if (err)
		return_error(err);

	/* For every vertex... */ 
	size_t i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];

		/* ...search all its outgoing edges */
		struct edge *e = v->head->next;
		while (e != &g->tail) {
			/* Skip edges that are part of the graph */
			if (e->removed == 0) {
				e = e->next;
				continue;
			}

			/* Create a new edge_entry */
			struct edge_entry *entry;
			err = list_new_entry(&entry, struct edge_entry, ln);
			if (err)
				goto fail;
			
			/* Fill entry */
			entry->src = v->segment;
			entry->dst = g->vertices[e->dst_id].segment;
			entry->dst_mapping = g->vertices[e->dst_id].mapping;
			entry->weight = e->weight;

			/* Append it to the list */
			err = list_insert_before(list_tail(*edges, struct edge_entry, ln),
					&entry->ln);
			if (err)
				goto fail;

			e = e->next;
		}
	}

	return 0;

fail:
	list_free(*edges, struct edge_entry, ln);
	return_error(err);
}

/**
 * Gets the vertices in topological order.
 *
 * The overlap graph must have no cycles for this to work. You can
 * use overlap_graph_remove_cycles() to remove them.
 *
 * @param g the overlap graph to get the vertices from
 * @param[out] vertices a list of struct vertex_entry topologically sorted
 *
 * @return the operation error code
 */
int overlap_graph_get_vertices_topo(overlap_graph_t *g, struct list **vertices)
{
	if (g == NULL || vertices == NULL)
		return_error(EINVAL);

	/* Create the list to hold the vertex entries */
	int err = list_new(vertices, struct vertex_entry, ln);
	if (err)
		return_error(err);

	/* Mark all the vertices as non visited */
	size_t i;
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];
		v->visited = 0;
	}

	/* 
	 * Visit all the vertices in a depth-first fashion.
	 */
	for (i = 0; i < g->size; i++) {
		struct vertex *v = &g->vertices[i];
		if (v->visited == 0) {
			err = topo_visit(g, i, *vertices);
			if (err)
				goto fail;
		}
	}

	return 0;

fail:
	list_free(*vertices, struct vertex_entry, ln);
	return_error(err);
}

