#ifndef SOUNDREX_RECEIVE_H
#define SOUNDREX_RECEIVE_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>

#include "../../magic_number.h"
#include "logger.h"

// Length at beginning of packet that is  useless.
// (Due to MAC header, right now.)
constexpr size_t max_buf_size = 3;
constexpr std::chrono::milliseconds max_play_at_end(50);
constexpr std::chrono::microseconds duration(packet_samples * 1000000ull / samples_per_s);

constexpr int max_repeat = max_play_at_end / duration;

typedef std::array<uint8_t, byte_depth> mono_sample_t;
typedef std::array<mono_sample_t, num_channels> sample_t;

struct raw_packet_t {
	uint8_t const *data;
	size_t size;
};

struct batch_t {
	uint32_t num;
	std::array<sample_t, packet_samples> samples;
	std::array<sample_t, trailing_samples> trailing;
};

void write_samples(void const *samples, size_t len);
bool receive_callback(raw_packet_t packet, logger_t &logger);
void play_next(logger_t &);

extern std::atomic<bool> correction_on;
#endif /* SOUNDREX_RECEIVE_H */
