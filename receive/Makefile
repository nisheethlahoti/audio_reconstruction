SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)

CC=g++
CXXFLAGS=-std=c++1z -Ofast -flto
LDLIBS=-lpcap -lasound
LDFLAGS=-pthread $(CXXFLAGS)

callbacks: $(OBJS)

clean:
	rm -f callbacks *.o

