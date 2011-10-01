#!/bin/sh

# Run test in valgrind.

BUILD_DIR=../build/visibility-public

if [ ! -f $BUILD_DIR/bindings/_libbls.so ]; then
	echo "You need to run './waf test' first to build the test infrastructure."
	exit
fi

LD_LIBRARY_PATH=$BUILD_DIR/src PYTHONPATH=$BUILD_DIR/bindings valgrind --leak-check=full python $1
