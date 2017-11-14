CC=g++
CXXFLAGS=-std=c++1z -O2
LDLIBS=-lpcap

send_song:send_song.o

clean:
	rm -f send_song send_song.o

