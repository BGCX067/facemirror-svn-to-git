YACCSRC=face.y
LEXSRC=face.l
CXX=g++
YACC=bison
LEX=flex
YACCFLAG=-d
LEXFLAG=
YACCCPPSRC=face.tab.cc
YACCHEADER=face.tab.hh
LEXCPPSRC=lex.yy.cc
SRC=$(YACCCPPSRC) $(LEXCPPSRC)
OBJ=$(patsubst %.cc,%.o,$(SRC))
LIBS=-ll
CXXFLAGS=-g
PROG=face2cpp

all: $(PROG)

$(PROG): $(OBJ)
	$(CXX) $(OBJ) $(LIBS) $(CXXFLAGS) -o $(PROG)

$(YACCCPPSRC): $(YACCSRC)
	$(YACC) $(YACCFLAG) $(YACCSRC) -o $(YACCCPPSRC)

$(LEXCPPSRC): $(LEXSRC)
	$(LEX) $(LEXFLAG) $(LEXSRC)
	mv lex.yy.c $(LEXCPPSRC)

clean:
	rm -f *.o
	rm -f *~
	rm -f $(SRC) $(YACCHEADER)
	rm -f $(PROG)