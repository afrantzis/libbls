#!/bin/sh

# Run test in valgrind.

LD_LIBRARY_PATH=../build/default/src PYTHONPATH=../build/default/bindings valgrind --leak-check=full python $1
