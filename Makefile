CXX ?= clang++
CXXFLAGS ?= -std=c++17 -O3 -march=native -DNDEBUG -Wall -Wno-sign-compare
CADICAL = third_party/cadical
KISSAT = third_party/kissat
INC = -I$(CADICAL)/src -I$(KISSAT)/src -Isrc
LIBS = $(CADICAL)/build/libcadical.a $(KISSAT)/build/libkissat.a

all: bin/nonosolve

bin/nonosolve: src/*.h src/*.cpp $(LIBS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INC) -o $@ src/main.cpp src/sat.cpp $(LIBS)

clean:
	rm -rf bin
