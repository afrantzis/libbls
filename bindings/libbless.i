%module libbless

%include "typemaps.i"

%{
#include "segment.h"
#include "segcol.h"
#include "segcol_list.h"
#include "data_object.h"
#include "data_object_memory.h"
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

/* Make data_object_read() binding output a PyBuffer */
%typemap(argout) (data_object_t *obj, void **buf, off_t offset, size_t len)
{
    %append_output(PyBuffer_FromMemory(*((unsigned char **)$2), $4));
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
%}

/* 
 * Make the data_object_write() binding accept as data input objects that
 * support the PyBuffer interface.
 */
%exception data_object_write
{
    ssize_t s;
    arg3 = get_read_buf_pyobj(obj2, &s);

    if (s >= arg4) {
        arg4 = s;
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
%apply unsigned long long  *OUTPUT { size_t * };

%include "../src/segment.h"
%include "../src/segcol.h"
%include "../src/segcol_list.h"
%include "../src/data_object.h"
%include "../src/data_object_memory.h"
%include "../src/buffer.h"


