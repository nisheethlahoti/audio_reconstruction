SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp, out/%.o, $(SRCS))
DEPS=$(OBJS:.o=.d)

CC=g++
CXXFLAGS=-std=gnu++1z -Ofast -flto -MMD -MP
LDLIBS=-lpcap -lasound
LDFLAGS=-pthread $(CXXFLAGS)

all: out out/callbacks

out/callbacks: $(OBJS)

out/%.o: src/%.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

out:
	mkdir -p out/

-include $(DEPS)

clean:
	rm -rf out/

.PHONY: all clean
