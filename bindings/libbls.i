%module libbls

%include "typemaps.i"
%include "cpointer.i"

%{
#include "type_limits.h"
#include "segment.h"
#include "segcol.h"
#include "segcol_list.h"
#include "data_object.h"
#include "data_object_memory.h"
#include "data_object_file.h"
#include "buffer.h"
#include "buffer_internal.h"
#include "priority_queue.h"
#include "overlap_graph.h"
#include "disjoint_set.h"
#include "buffer_util.h"
#include "list.h"
%}

%pointer_class (size_t, size_tp)


%apply long long { ssize_t };
%apply unsigned long long { size_t };
%apply long long { off_t };

/* Don't perform any conversions on void pointers */
%typemap(in) void *
{
    $1 = $input;
}

/* Handle bless_buffer_source_t normally. This is needed because
 * bless_buffer_source_t is typedef void and the previous rule is
 * used (but we don't want to).
 */
%typemap(in) bless_buffer_source_t * = SWIGTYPE *;

/*
 * The typemaps below are used to handle functions that return values in arguments.
 * If a function is of the form 'int func(type **t, type1 p)' the resulting binding 
 * is of the form (int, type *) = func(type1).
 */

/*
 * Typemaps that handle double pointers.
 */

/* Essentially ignore input */
%typemap(in,numinputs=0) segment_t ** ($*1_type retval)
{
    /* in */
    retval = NULL;
    $1 = &retval;
}

/* Append segment_t * to return list */
%typemap(argout) segment_t ** 
{
    %append_output(SWIG_NewPointerObj(SWIG_as_voidptr(retval$argnum), $*1_descriptor, 0));
}

/* The same rules for segment_t ** apply to other ** types */
%apply segment_t ** { segcol_t ** , segcol_iter_t **, data_object_t **, void **}
%apply segment_t ** { bless_buffer_t **, bless_buffer_source_t ** }
%apply segment_t ** { priority_queue_t **, overlap_graph_t **, disjoint_set_t ** }
%apply segment_t ** { struct list ** }


/* Exception for void **: Append void * to return list without conversion */
%typemap(argout) void ** 
{
    %append_output((PyObject *)retval$argnum);
    Py_INCREF((PyObject *)retval$argnum);
}

/* Match segment_new() and increase reference count of stored data */
%typemap(check) (segment_t **seg, void *data)
{
    if ($2 != NULL)
        Py_INCREF((PyObject *)$2);
}

/* Decrease reference count of data stored in a segment_t when it is freed */
%exception segment_free
{
    if (arg1 != NULL) {
        void *data = NULL;
        segment_get_data(arg1, &data);
        if (data != NULL) {
            Py_DECREF((PyObject *)data);
        }
    }
    $action
}

/* handle 'length' in data_object_get_data() */
%typemap(in) off_t *length = size_t *INPUT;

/* Make data_object_get_data() binding output a PyBuffer */
%typemap(argout) (data_object_t *obj, void **buf, off_t offset,
off_t *length, data_object_flags flags)
{
    if (($5 | DATA_OBJECT_RW) != 0)
        %append_output(PyBuffer_FromReadWriteMemory(*((unsigned char **)$2), *$4));
    else
        %append_output(PyBuffer_FromMemory(*((unsigned char **)$2), *$4));
}

%{
/* Gets a pointer to a copy of the first segment of raw data of a PyBuffer */
void *get_read_buf_pyobj_copy(PyObject *obj, ssize_t *size)
{
    PyTypeObject *tobj = obj->ob_type;

    PyBufferProcs *procs = tobj->tp_as_buffer;

    /* if object does not support the buffer interface... */
    if (procs == NULL)
        return NULL;

    readbufferproc proc = procs->bf_getreadbuffer;

    void *ptr;

    *size = (*proc)(obj, 0, &ptr);

    if (*size == -1)
        return NULL;
        
    void *data_copy = malloc(*size);
    memcpy(data_copy, ptr, *size);

    return data_copy;
}

/* Gets a write pointer to the first segment of raw data of a PyBuffer */
void *get_write_buf_pyobj(PyObject *obj, ssize_t *size)
{
    PyTypeObject *tobj = obj->ob_type;

    PyBufferProcs *procs = tobj->tp_as_buffer;

    /* if object does not support the buffer interface... */
    if (procs == NULL)
        return NULL;

    writebufferproc proc = procs->bf_getwritebuffer;

    void *ptr;

    *size = (*proc)(obj, 0, &ptr);

    return ptr;
}
%}

/* 
 * Make the bless_buffer_append() binding accept as data input objects that
 * support the PyBuffer interface.
 */
%exception bless_buffer_source_memory
{
    ssize_t s;
    arg2 = get_read_buf_pyobj_copy(obj0, &s);

    if (s != -1 && s >= arg3) {
        $action
    }
    else
        result = 666;
}

/* 
 * Make the bless_buffer_read() binding accept as data input objects that
 * support the PyBuffer interface.
 */
%exception bless_buffer_read
{
    ssize_t s;

    arg3 = get_write_buf_pyobj(obj2, &s);

    if (s != -1 && s >= arg5) {
        $action
    }
    else
        result = 666;
}

/* 
 * Make the read_data_object() binding accept as data input objects that
 * support the PyBuffer interface.
 */
%exception read_data_object
{
    ssize_t s;

    arg3 = get_write_buf_pyobj(obj2, &s);

    if (s != -1 && s >= arg4) {
        $action
    }
    else
        result = 666;
}

/*
 * Typemaps that handle output arguments.
 */

%apply int *OUTPUT { int * };
%apply long long *OUTPUT { off_t * };
%apply unsigned long long *OUTPUT { size_t * };

/* in priority_queue_add size_t *pos is a normal pointer (not output) */
%apply SWIGTYPE * { size_t *pos };

/*
 * Helper functions available only in python code for testing purposes.
 *
 * These should be placed at the end of the SWIG file so the typemaps 
 * will apply to them.
 */
%inline %{
/* Get the maximum value of off_t */
off_t get_max_off_t(void)
{
    return __MAX(off_t);
}

/* Get the maximum value of size_t */
size_t get_max_size_t(void)
{
    return __MAX(size_t);
}

/* Create a segment with the data pointer as size_t */
int segment_new_ptr(segment_t **seg, size_t ptr, off_t start, off_t size,
segment_data_usage_func data_usage_func)
{
    return segment_new(seg, (void *)ptr, start, size, data_usage_func);
}

/* Set the segcol of a bless_buffer_t */
void set_buffer_segcol(bless_buffer_t *buf, segcol_t *segcol)
{
    free(buf->segcol);
    buf->segcol = segcol;
}

/* Call data_object_memory_new with the data pointer as size_t */
int data_object_memory_new_ptr(data_object_t **o, size_t ptr, size_t len)
{
    return data_object_memory_new(o, (void *)ptr, len);
}

/* Call bless_buffer_source_memory with the data pointer as size_t */
int bless_buffer_source_memory_ptr(bless_buffer_source_t **src, size_t ptr, size_t len, bless_mem_free_func *func)
{
    return bless_buffer_source_memory(src, (void *)ptr, len, func);
}

/* Call bless_buffer_read with the data pointer as size_t */
int bless_buffer_read_ptr(bless_buffer_t *src, off_t src_offset, size_t dst,
size_t dst_offset, size_t length)
{
    return bless_buffer_read(src, src_offset, (void *)dst, dst_offset, length);
}

/* Append a segment to buffer */ 
int bless_buffer_append_segment(bless_buffer_t *buf, segment_t *seg)
{
    if (buf == NULL || seg == NULL) 
        return EINVAL;

    segcol_t *sc = buf->segcol;

    /* Append segment to the segcol */
    int err = segcol_append(sc, seg);
    if (err) {
        return err;
    }

    return 0;
}

/* Malloc memory and return the address cast to size_t */
size_t bless_malloc(size_t s)
{
    void *p = malloc(s);

    return (size_t)p;
}

/* Delete a range from a segment but don't return the deleted segments */
int segcol_delete_no_deleted(segcol_t *segcol, off_t offset, off_t length)
{
    return segcol_delete(segcol, NULL, offset, length);
}

/* 
 * Prints a list of segment edges assuming that the segment data 
 * is a PyString value.
 */
void print_edge_list(struct list *edges, int fd)
{
    FILE *fp = fdopen(fd, "w");
    struct list_node *node;

    list_for_each(list_head(edges, struct edge_entry, ln)->next, node) {
        struct edge_entry *e = list_entry(node, struct edge_entry, ln);
        PyObject *str1;
        segment_get_data(e->src, (void **)&str1);
        PyObject *str2;
        segment_get_data(e->dst, (void **)&str2);

        fprintf(fp, "%s -> %s\n", PyString_AsString(str1),
            PyString_AsString(str2));
    }

    fclose(fp);
}

/* 
 * Prints a list of segment vertices assuming that the segment data 
 * is a PyString value.
 */
void print_vertex_list(struct list *vertices, int fd)
{
    FILE *fp = fdopen(fd, "w");
    struct list_node *node;

    list_for_each(list_head(vertices, struct vertex_entry, ln)->next, node) {
        struct vertex_entry *e = list_entry(node, struct vertex_entry, ln);
        PyObject *str1;
        segment_get_data(e->segment, (void **)&str1);

        fprintf(fp, "%s\n", PyString_AsString(str1));
    }

    fclose(fp);
}
%}

%include "../src/segment.h"
%include "../src/segcol.h"
%include "../src/segcol_list.h"
%include "../src/data_object.h"
%include "../src/data_object_memory.h"
%include "../src/data_object_file.h"
%include "../src/buffer.h"
%include "../src/buffer_source.h"
%include "../src/priority_queue.h"
%include "../src/overlap_graph.h"
%include "../src/disjoint_set.h"
%include "../src/buffer_util.h"
%include "../src/list.h"

