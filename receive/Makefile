SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp, out/%.o, $(SRCS))
DEPS=$(OBJS:.o=.d)

CC=g++
CXXFLAGS=-std=gnu++1z -Ofast -flto
MFLAGS=-MMD -MP
LDLIBS=-lpcap
LDFLAGS=-pthread $(CXXFLAGS)

all: out out/callbacks out/play

out/callbacks: $(OBJS)

out/%.o: src/%.cpp
	$(CC) $(CXXFLAGS) $(MFLAGS) -c $< -o $@

out:
	mkdir -p out/

out/play: play/alsa_play.cpp
	$(CC) $(CXXFLAGS) $(MFLAGS) $< -o $@ -lasound

-include $(DEPS)
-include out/play.d

parse/readlog: parse/readlog.cpp src/logger.cpp
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -rf out/
	rm parse/readlog

.PHONY: all clean
