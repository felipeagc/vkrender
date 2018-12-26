main: lexer.l parser.y
	bison -d -o parser.cpp parser.y
	flex -o lexer.cpp lexer.l

clean:
	rm -f lexer.cpp parser.cpp parser.hpp location.hh
