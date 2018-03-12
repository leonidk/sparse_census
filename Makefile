CC ?= gcc
CXX ?= g++

CPPFLAGS = -I../src `pkg-config --cflags hiredis`
CFLAGS = -g -std=gnu99 -Wall -Wno-unused-parameter -Wno-unused-function -O2
CXXFLAGS = -g -Wall -std=c++14 #-O2 -march=native
LDFLAGS = -lpthread -lm `pkg-config --libs hiredis`

TARGETS := test_sparse

.PHONY: all
all: test_sparse

test_sparse: test.o 
	@mkdir -p ../bin
	@echo "   [$@]"
	@$(CXX) -o $@ $^ $(LDFLAGS) `pkg-config --libs opencv`

%.o: %.c
	@echo "   $@"
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

%.o: %.cpp sparse_census.h
	@echo "   $@"
	@$(CXX) -o $@ -c $< $(CXXFLAGS) $(CPPFLAGS)

.PHONY: clean
clean:
	@rm -rf *.o $(TARGETS)
