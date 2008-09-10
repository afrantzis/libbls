%module libbless

%include "typemaps.i"

%{
#include "segment.h"
#include "segcol.h"
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
%apply segment_t ** { segcol_t ** , segcol_iter_t **, void **}

/* Exception for void **: Append void * to return list without conversion */
%typemap(argout) void ** 
{
    %append_output((PyObject *)retval$argnum);
}

/*
 * Typemaps that handle double pointers.
 */

%apply long long *OUTPUT { off_t * };
%apply unsigned long long  *OUTPUT { size_t * };

%include "../src/segment.h"
%include "../src/segcol.h"
%include "../src/buffer.h"

