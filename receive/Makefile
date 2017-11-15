SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp, out/%.o, $(SRCS))

CC=g++
CXXFLAGS=-std=gnu++1z -Ofast -flto
LDLIBS=-lpcap -lasound
LDFLAGS=-pthread $(CXXFLAGS)

all: init out/callbacks

out/callbacks: $(OBJS)

out/%.o: src/%.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

init:
	mkdir -p out/

clean:
	rm -rf out/

.PHONY: all init clean
