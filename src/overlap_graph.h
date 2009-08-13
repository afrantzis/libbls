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
 * libbls.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file overlap_graph.h
 *
 * Overlap graph API
 */
#ifndef _BLESS_OVERLAP_GRAPH_H
#define _BLESS_OVERLAP_GRAPH_H

#include "segment.h"
#include "list.h"
#include <sys/types.h>

/**
 * @defgroup overlap_graph Overlap Graph
 *
 * Graph showing the overlap of segments.
 *
 * See doc/devel/buffer_save.txt for more information.
 *
 * @{
 */

/**
 * Opaque type for overlap graph.
 */
typedef struct overlap_graph overlap_graph_t;

/**
 * List entry for edges
 */
struct edge_entry {
	struct list_node ln;
	segment_t *src;
	segment_t *dst;
	off_t dst_mapping;
	off_t weight;
};

/**
 * List entry for segments
 */
struct vertex_entry {
	struct list_node ln;
	segment_t *segment;
	off_t mapping;
	off_t self_loop_weight;
};

int overlap_graph_new(overlap_graph_t **g, size_t capacity);

int overlap_graph_free(overlap_graph_t *g);

int overlap_graph_add_segment(overlap_graph_t *g, segment_t *seg, off_t mapping); 

int overlap_graph_remove_cycles(overlap_graph_t *g);

int overlap_graph_get_removed_edges(overlap_graph_t *g, list_t **edges);

int overlap_graph_get_vertices_topo(overlap_graph_t *g, list_t **vertices);

int overlap_graph_export_dot(overlap_graph_t *g, int fd);

/** @} */

#endif
