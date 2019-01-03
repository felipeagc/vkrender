#!/bin/zsh

wc -l \
	./engine/**/*.hpp \
	./engine/**/*.cpp \
	./renderer/**/*.cpp \
	./renderer/**/*.hpp \
	./examples/*.cpp \
	./ftl/**/*.hpp \
	./shaders/*.vert \
	./shaders/*.frag \
	| sort -n -r -k1
