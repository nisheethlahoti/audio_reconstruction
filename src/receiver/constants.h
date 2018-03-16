#ifndef SOUNDREX_RECEIVE_H
#define SOUNDREX_RECEIVE_H

#include <chrono>
#include <cstdint>

#include <constants.h>

constexpr std::chrono::milliseconds max_play_at_end(50);
constexpr std::chrono::microseconds duration(packet_samples * 1000000ull / samples_per_s);

constexpr int max_repeat = max_play_at_end / duration;

typedef std::array<uint8_t, byte_depth> mono_sample_t;
typedef std::array<mono_sample_t, num_channels> sample_t;

struct raw_packet_t {
	uint8_t const *data;
	ssize_t size;
};

#endif /* SOUNDREX_RECEIVE_H */
