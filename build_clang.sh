#!/bin/sh
mkdir build && cd build
CC=clang CXX=clang++ cmake -G Ninja ..
cd ..
