wc -l \
	./engine/**/*.hpp \
	./engine/**/*.cpp \
	./renderer/**/*.cpp \
	./renderer/**/*.hpp \
	./examples/*.cpp \
	./fstl/**/*.hpp \
	| sort -n -r -k1
