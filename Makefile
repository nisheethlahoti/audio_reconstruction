CC=g++
CXXFLAGS=-std=gnu++1z -O2
LDLIBS=-lpcap

send_song:send_song.o

clean:
	rm -f send_song send_song.o
