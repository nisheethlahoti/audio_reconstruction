rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
objects=$(patsubst src/%.cpp, out/%.o, $(wildcard src/$1/*.cpp))

DEPS=$(patsubst src/%.cpp, out/%.d, $(call rwildcard,src/,*.cpp))
RECV_TARGETS=out/receive out/alsa_play out/readlog
TRSM_TARGETS=out/transmit out/packetize out/throttle

CXX=g++
COMMONFLAGS=-std=gnu++1z -Ofast -flto
CXXFLAGS=$(COMMONFLAGS) -I ./src/ -MMD -MP
LDFLAGS=-pthread $(COMMONFLAGS)

all:
	@echo "Please specify which target to build: receiver transmitter $(RECV_TARGETS) $(TRSM_TARGETS)"

out/receive: LDLIBS=-lpcap
out/receive: $(call objects,receiver) $(call objects,receiver/unix) out/realtime.o

out/alsa_play: LDLIBS=-lasound
out/alsa_play: out/receiver/play/alsa_play.o out/realtime.o

out/transmit: LDLIBS=-lpcap
out/transmit: out/transmitter/send_song.o out/realtime.o

out/readlog: out/receiver/logtypes.o out/receiver/readlog/readlog.o
out/packetize: out/transmitter/packetize.o out/realtime.o
out/throttle: out/transmitter/throttle.o out/realtime.o

$(RECV_TARGETS) $(TRSM_TARGETS):
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

receiver: $(RECV_TARGETS)
transmitter: $(TRSM_TARGETS)

out/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf out/

.PHONY: all clean receiver transmitter
