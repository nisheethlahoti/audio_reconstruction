rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
get_objs=$(patsubst src/%.cpp, obj/%.o, $1)
get_execs_from=$(patsubst src/unix/%$1, exec/%, $(wildcard src/unix/*$1))

dir_bins=$(filter-out exec/soundrex, $(call get_execs_from,/.))
cpp_bins=$(call get_execs_from,.cpp)
objs=$(call get_objs,$(call rwildcard,src/,*.cpp))
deps=$(objs:.o=.d)

all: $(cpp_bins) $(dir_bins)

receiver: exec/receive exec/process exec/alsa_play exec/readlog
transmitter: exec/transmit exec/packetize exec/throttle

CXX=clang++
COMMONFLAGS=-std=gnu++1z -pthread -Ofast -flto
CXXFLAGS=$(COMMONFLAGS) -I ./src/ -MMD -MP
LDFLAGS=$(COMMONFLAGS)

$(dir_bins) $(cpp_bins): obj/unix/soundrex/common.o
exec/transmit exec/receive: obj/unix/soundrex/capture.o
exec/transmit exec/receive: LDLIBS+=-lpcap
exec/alsa_play: LDLIBS+=-lasound
exec/process: obj/soundrex/processor.o

obj/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

exec/%:
	@mkdir -p exec
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include $(deps)

clean:
	rm -rf obj
	rm -rf exec

.SECONDEXPANSION:
$(dir_bins): exec/%: $$(call get_objs,$$(wildcard src/unix/%/*.cpp))
$(cpp_bins): exec/%: $$(call get_objs,src/unix/%.cpp)

.PHONY: all clean receiver transmitter
