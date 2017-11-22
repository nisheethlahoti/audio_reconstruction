CC=g++
CXXFLAGS=-std=gnu++1z -O2 -MMD -MP
LDLIBS=-lpcap

send_song:send_song.o

-include send_song.d

clean:
	rm -f send_song send_song.o
