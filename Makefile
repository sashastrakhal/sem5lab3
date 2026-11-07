CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wno-unused-function

all: compiler

compiler: parser.tab.cpp lex.yy.c codegen.cpp ast.hpp
	$(CXX) $(CXXFLAGS) parser.tab.cpp lex.yy.c codegen.cpp -o compiler

parser.tab.cpp parser.tab.hpp: parser.y
	bison -d -v -o parser.tab.cpp parser.y

lex.yy.c: lexer.l parser.tab.hpp
	flex -o lex.yy.c lexer.l

run: compiler
	./compiler test.c

clean:
	rm -f compiler parser.tab.cpp parser.tab.hpp parser.output lex.yy.c output.py output.tac ast_visual.html