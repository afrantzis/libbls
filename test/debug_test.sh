#!/bin/sh

# Run test in gdb. Make sure to have compiled the code (and optionally the bindings)
# with debug symbols (scons debug=1)

LD_LIBRARY_PATH=../build/src PYTHONPATH=../build/bindings gdb --args python $1
