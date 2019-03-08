#!/bin/zsh

wc -l \
	./engine/**/*.hpp \
	./engine/**/*.cpp \
	./renderer/**/*.cpp \
	./renderer/**/*.hpp \
	./util/**/*.h \
	./examples/*.cpp \
	./shaders/*.vert \
	./shaders/*.frag \
	| sort -n -r -k1
