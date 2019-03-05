#!/bin/zsh

wc -l \
	./engine/**/*.hpp \
	./engine/**/*.cpp \
	./renderer/**/*.cpp \
	./renderer/**/*.hpp \
	./util/**/*.h \
	./util/**/*.c \
	./examples/*.cpp \
	./shaders/*.vert \
	./shaders/*.frag \
	| sort -n -r -k1
