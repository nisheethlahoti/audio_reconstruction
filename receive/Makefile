SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp, out/%.o, $(SRCS)) ../realtime.o
DEPS=$(OBJS:.o=.d)

CC=g++
CXXFLAGS=-std=gnu++1z -Ofast -flto
MFLAGS=-MMD -MP
out/callbacks: LDLIBS=-lpcap
out/play: LDLIBS=-lasound
LDFLAGS=-pthread $(CXXFLAGS)

all: out out/callbacks out/play

out/callbacks: $(OBJS)

../realtime.o: ../realtime.cpp
	$(CC) $(CXXFLAGS) $(MFLAGS) -c $< -o $@

out/play.o: play/alsa_play.cpp
	$(CC) $(CXXFLAGS) $(MFLAGS) -c $< -o $@

out/%.o: src/%.cpp
	$(CC) $(CXXFLAGS) $(MFLAGS) -c $< -o $@

out:
	mkdir -p out/

out/play: out/play.o ../realtime.o

-include $(DEPS)
-include out/play.d

parse/readlog: parse/readlog.cpp src/logger.cpp
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	rm -rf out/
	rm parse/readlog

.PHONY: all clean
