#!/bin/zsh

wc -l \
	./engine/**/*.hpp \
	./engine/**/*.cpp \
	./renderer/**/*.cpp \
	./renderer/**/*.hpp \
	./examples/*.cpp \
	./ftl/**/*.hpp \
	./sdf/**/*.hpp \
	./sdf/**/*.cpp \
	./shaders/*.vert \
	./shaders/*.frag \
	| sort -n -r -k1
