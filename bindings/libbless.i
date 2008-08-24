%module libbless

%include "typemaps.i"

%{
#include "segment.h"
#include "segcol.h"
#include "buffer.h"
%}


/*
 * The typemaps below are used to handle functions that return values in arguments.
 * If a function is of the form 'int func(type **t)' the resulting binding 
 * is of the form (int, type *) = func().
 */

/*
 * Constructor typemaps that handle double pointers.
 */

/* Essentially ignore input. */
%typemap(in,numinputs=0) segment_t ** (segment_t *retval)
{
    /* segment_t ** in */
    retval = NULL;
    $1 = &retval;
}

/* Append segment_t * to return list */
%typemap(argout) segment_t ** 
{
    %append_output(SWIG_NewPointerObj(SWIG_as_voidptr(retval$argnum), $*1_descriptor, 0));
}

%apply long long { ssize_t };
%apply unsigned long long { size_t };
%apply long long { off_t };


%apply long long *OUTPUT { off_t *};
%apply unsigned long long  *OUTPUT { size_t *};

%include "../src/segment.h"
%include "../src/segcol.h"
%include "../src/buffer.h"

