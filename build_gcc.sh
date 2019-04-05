#!/bin/sh
mkdir build && cd build
CC=gcc CXX=g++ cmake -G Ninja ..
cd ..
