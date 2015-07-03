LEX = flex
YACC = bison
CXX = g++

all:
	$(LEX) dragon.l
	$(YACC) dragon.yy -d
	$(CXX) dragon.tab.cc lex.yy.c -o grammar

clean:
	rm -f *.hh *.cc *.yy.c grammar
