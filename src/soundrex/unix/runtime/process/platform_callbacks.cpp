#include <soundrex/platform_callbacks.h>
#include "logger.h"

static logger_t packetlogger("packet.bin"), playlogger("play.bin");

template <class log_t>
void packet_log(log_t log) {
	packetlogger.log(log);
}

template <class log_t>
void play_log(log_t log) {
	playlogger.log(log);
}

void write_samples(sample_t const *samples, size_t len) {
	buf_write(samples, len * sizeof sample_t());
}

#define LOG_TYPE(_, NAME, ...)                                        \
	template void packet_log<CONCAT(NAME, _log)>(CONCAT(NAME, _log)); \
	template void play_log<CONCAT(NAME, _log)>(CONCAT(NAME, _log));

#include <soundrex/logtypes.list>
