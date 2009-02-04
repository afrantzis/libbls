#!/bin/sh

# Run test in valgrind.

LD_LIBRARY_PATH=../build/src PYTHONPATH=../build/bindings valgrind --leak-check=full python $1
