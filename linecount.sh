wc -l \
	./engine/**/*.hpp \
	./engine/**/*.cpp \
	./renderer/**/*.cpp \
	./renderer/**/*.hpp \
	./examples/*.cpp \
	./fstl/**/*.hpp \
	./shaders/*.vert \
	./shaders/*.frag \
	| sort -n -r -k1
