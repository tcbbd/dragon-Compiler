LEX = flex
YACC = bison
CXX = g++
CXXFLAGS = -std=c++11

LEXOBJS = lex.yy.c
YACCOBJS = dragon.tab.cc dragon.tab.hh location.hh position.hh stack.hh
OBJECTS = dragon.o ast.o dragon.tab.o lex.yy.o

all: dragon

$(LEXOBJS) : dragon.l
	$(LEX) dragon.l

$(YACCOBJS) : dragon.yy
	$(YACC) dragon.yy -d

lex.yy.o : lex.yy.c ast.h $(YACCOBJS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o : %.cc ast.h $(LEXOBJS) $(YACCOBJS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

dragon: $(OBJECTS)
	$(CXX) $^ -o $@

clean:
	rm -f *.o $(LEXOBJS) $(YACCOBJS) dragon
