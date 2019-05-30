#!/bin/zsh

wc -l \
	./engine/**/*.h \
	./engine/**/*.c \
	./renderer/renderer/**/*.c \
	./renderer/renderer/**/*.h \
	./examples/*.c \
	./thirdparty/gmath/**/*.h \
	./thirdparty/fstd/**/*.h \
	./thirdparty/fstd/**/*.c \
	./shaders/*.vert \
	./shaders/*.frag \
	| sort -n -r -k1
