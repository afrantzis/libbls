%module libbless
%{
#include "segment.h"
#include "segcol.h"
#include "buffer.h"
%}

%apply long long { ssize_t };
%apply unsigned long long { size_t };
%apply long long { off_t };

%include "../src/segment.h"
%include "../src/segcol.h"
%include "../src/buffer.h"

