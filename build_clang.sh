#!/bin/sh
CC=clang CXX=clang++ cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -G Ninja .
