#!/bin/zsh

wc -l \
	./engine/**/*.h \
	./engine/**/*.c \
	./renderer/**/*.c \
	./renderer/**/*.h \
	./util/**/*.h \
	./examples/*.c \
	./shaders/*.vert \
	./shaders/*.frag \
	| sort -n -r -k1
