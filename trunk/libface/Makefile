CXX=g++
CPPFLAGS=-Wall -O0 -ggdb -I/usr/local/include -I.
# here libs_r is the thread-safe library, if it spits error,
#you might wanna recompile your mysql client library
CLIBS=-levent -lssl
SRC=$(wildcard *.cpp) 
OBJ=$(patsubst %.cpp,%.o,$(SRC))
DEP=.depend
PROG=fa

all: $(PROG)

$(PROG): $(OBJ)
	$(CXX) $(CPPFLAG) $(OBJ) $(CLIBS) -o $(PROG)


# $(DEP): $(SRC)
# 	$(CXX) -MM $(SRC) $(CPPFLAGS) > $(DEP)

# $(OBJ)): $(SRC)
# 	$(CXX) $(CPPFLAG) $(OBJ) $(CLIBS) -o $(PROG)

s:
	$(CXX) -DMAKEEXE -DMAKESERVER -g -c face.cpp
	$(CXX) $(CPPFLAG) $(OBJ) $(CLIBS) -o $(PROG)

c:
	$(CXX) -DMAKEEXE -DMAKECLI -g -c face.cpp
	$(CXX) $(CPPFLAG) $(OBJ) $(CLIBS) -o $(PROG)

a:$(OBJ)
	ar r libface.a $(OBJ)

clean: 
	rm -f $(PROG)
	rm -f $(OBJ)
	rm -f *~
	rm -f libface.a
