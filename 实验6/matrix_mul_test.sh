#!/bin/bash
set -e
mkdir -p bin

# Compile the parallel_for shared library
clang -fPIC -shared -o bin/libparallel_for.so parallel_for.c -pthread

# Compile the test program
clang -o bin/matrix_mul_test matrix_mul_test.c -I. -L./bin -lparallel_for -pthread -lm

# Update library path and run the test
export DYLD_LIBRARY_PATH=$(pwd)/bin:$DYLD_LIBRARY_PATH
./bin/matrix_mul_test
