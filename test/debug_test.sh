#!/bin/sh

# Run test in gdb. Make sure to have compiled the code (and optionally the bindings)
# with debug symbols (scons debug=1)

LD_LIBRARY_PATH=../build/default/src PYTHONPATH=../build/default/bindings gdb --args python $1
