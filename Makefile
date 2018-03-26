SRCROOT=src/soundrex
CXX=clang++
COMMONFLAGS=-std=gnu++1z -pthread -Ofast -flto
CXXFLAGS=$(COMMONFLAGS) -I src/ -MMD -MP -Wall -Wextra
LDFLAGS=$(COMMONFLAGS) -B/usr/lib/gold-ld

all:

exec/transmit exec/receive: obj/unix/lib/capture.o
exec/transmit exec/receive: LDLIBS+=-lpcap
exec/alsa_play: LDLIBS+=-lasound
exec/process: obj/lib/processor.o
exec/receive exec/packetize: obj/unix/lib/multiplexer.o

###################   COMPLETELY GENERIC SECTION HENCEFORTH   ####################

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
ancestors=$1 $(if $(filter-out $(SRCROOT),$1), $(call ancestors,$(patsubst %/,%,$(dir $1))))
get_obj=$(patsubst $(SRCROOT)/%, obj/%.o, $1)
get_exec=$(addprefix exec/,$(notdir $t))
sub_cpps=$(basename $(call rwildcard,$1/,*.cpp))
noexec=$(filter-out $1,$(filter $(cpps),$(call ancestors,$1)))$(filter %/lib,$(call ancestors,$1))

cpps=$(call sub_cpps,$(SRCROOT))
objs=$(call get_obj,$(cpps))
deps=$(objs:.o=.d)
dirs=$(patsubst %//., %, $(call rwildcard,$(SRCROOT)/,/.))
actives=$(foreach c,$(cpps),$(if $(call noexec,$c),,$c))
dir_targets=$(filter $(dirs),$(actives))

$(foreach t,$(dir_targets),$(eval $(call get_exec,$t): $(call get_obj,$(call sub_cpps,$t))))

$(foreach t,$(actives), $(eval\
	$(call get_exec,$t):$(call get_obj,$t $(addsuffix /lib,$(filter-out $t,$(call ancestors,$t))))\
))

all: $(foreach t,$(actives), $(call get_exec,$t))

obj/%.o: $(SRCROOT)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

exec/%:
	@mkdir -p exec
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include $(deps)

clean:
	rm -rf obj exec

.PHONY: all clean
