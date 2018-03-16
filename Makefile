CC=g++
CXXFLAGS=-std=gnu++1z -O2 -MMD -MP
send_song: LDLIBS=-lpcap

all: send_song packetize

send_song:send_song.o realtime.o

packetize: packetize.o realtime.o

-include send_song.d
-include packetize.d
-include realtime.d

clean:
	rm -f *.o *.d send_song packetize

.PHONY: all clean
