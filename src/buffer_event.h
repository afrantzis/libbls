/*
 * Copyright 2009 Alexandros Frantzis, Michael Iatrou
 *
 * This file is part of libbls.
 *
 * libbls is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
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
 * @file buffer_event.h
 *
 * Buffer events
 */
#ifndef _BLESS_BUFFER_EVENT_H
#define _BLESS_BUFFER_EVENT_H

/**
 * Buffer events.
 */
enum {
	BLESS_BUFFER_EVENT_NONE = 0,
	BLESS_BUFFER_EVENT_EDIT,    /**< The buffer was edited */
	BLESS_BUFFER_EVENT_UNDO,    /**< The last buffer action was undone */
	BLESS_BUFFER_EVENT_REDO,    /**< The last undone action was redone */
	BLESS_BUFFER_EVENT_SAVE,    /**< The buffer was saved */
	BLESS_BUFFER_EVENT_DESTROY, /**< The buffer is about to be destroyed */
};

/**
 * Buffer actions.
 *
 * These actions further specify BLESS_BUFFER_EVENT_EDIT,
 * BLESS_BUFFER_EVENT_UNDO, BLESS_BUFFER_EVENT_REDO events.
 */
enum {
	BLESS_BUFFER_ACTION_NONE = 0,
	BLESS_BUFFER_ACTION_APPEND, /**< Append action */
	BLESS_BUFFER_ACTION_INSERT, /**< Insert action */
	BLESS_BUFFER_ACTION_DELETE, /**< Delete action */
	BLESS_BUFFER_ACTION_MULTI,  /**< Multi action */
};

/**
 * Information about a buffer event.
 */
struct bless_buffer_event_info {
	int event_type;     /**< The event type (BLESS_BUFFER_EVENT_*) */
	int action_type;    /**< The action type (BLESS_BUFFER_ACTION_*) */
	off_t range_start;  /**< The start of the range of the buffer that
	                         was affected by an event */
	off_t range_length; /**< The length of the range of the buffer that
	                         was affected by an event */
	int save_fd;        /**< The descriptor of the file the buffer was
	                         saved to */
};

#endif /* _BLESS_BUFFER_EVENT_H */
