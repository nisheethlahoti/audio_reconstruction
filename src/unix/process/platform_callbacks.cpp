#include <alsa/asoundlib.h>
#include <soundrex/platform_callbacks.h>
#include <iostream>
#include "logger.h"

static logger_t packetlogger("packet.bin"), playlogger("play.bin");
extern snd_pcm_t *handle;

template <class log_t>
void packet_log(log_t log) {
	packetlogger.log(log);
}

template <class log_t>
void play_log(log_t log) {
	playlogger.log(log);
}

void write_samples(sample_t const *samples, size_t len) {
	snd_pcm_sframes_t frames;
	while (frames = snd_pcm_writei(handle, samples, len), frames < 0)
		snd_pcm_recover(handle, frames, 0);

	if (frames < len)
		std::cerr << "Short write (expected " << len << ", wrote " << frames << ")\n";
}

#define LOG_TYPE(_, NAME, ...)                                        \
	template void packet_log<CONCAT(NAME, _log)>(CONCAT(NAME, _log)); \
	template void play_log<CONCAT(NAME, _log)>(CONCAT(NAME, _log));

#include <soundrex/logtypes.list>
