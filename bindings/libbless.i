%module libbless

%include "typemaps.i"

%{
#include "segment.h"
#include "segcol.h"
#include "segcol_list.h"
#include "data_object.h"
#include "data_object_memory.h"
#include "data_object_file.h"
#include "buffer.h"
%}

%apply long long { ssize_t };
%apply unsigned long long { size_t };
%apply long long { off_t };

/* Don't perform any conversions on void pointers */
%typemap(in) void *
{
    $1 = $input;
}

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
%apply segment_t ** { bless_buffer_t ** }

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
    $action
    if (arg1 != NULL) {
        void *data = NULL;
        segment_get_data(arg1, &data);
        if (data != NULL)
            Py_DECREF((PyObject *)data);
    }
}

/* handle 'length' in data_object_get_data() */
%typemap(in) size_t *length = size_t *INPUT;

/* Make data_object_get_data() binding output a PyBuffer */
%typemap(argout) (data_object_t *obj, void **buf, off_t offset,
size_t *length, data_object_flags flags)
{
    if (($5 | DATA_OBJECT_RW) != 0)
        %append_output(PyBuffer_FromReadWriteMemory(*((unsigned char **)$2), *$4));
    else
        %append_output(PyBuffer_FromMemory(*((unsigned char **)$2), *$4));
}

%{
/* Gets a pointer to the first segment of raw data of a PyBuffer */
void *get_read_buf_pyobj(PyObject *obj, ssize_t *size)
{
    PyTypeObject *tobj = obj->ob_type;

    PyBufferProcs *procs = tobj->tp_as_buffer;

    /* if object does not support the buffer interface... */
    if (procs == NULL)
        return NULL;

    readbufferproc proc = procs->bf_getreadbuffer;

    void *ptr;

    *size = (*proc)(obj, 0, &ptr);

    return ptr;
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
%exception bless_buffer_append
{
    ssize_t s;
    arg2 = get_read_buf_pyobj(obj1, &s);

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
 * Typemaps that handle output arguments.
 */

%apply int *OUTPUT { int * };
%apply long long *OUTPUT { off_t * };
%apply unsigned long long *OUTPUT { size_t * };

%include "../src/segment.h"
%include "../src/segcol.h"
%include "../src/segcol_list.h"
%include "../src/data_object.h"
%include "../src/data_object_memory.h"
%include "../src/data_object_file.h"
%include "../src/buffer.h"


