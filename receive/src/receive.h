#ifndef SOUNDREX_RECEIVE_H
#define SOUNDREX_RECEIVE_H

#include <chrono>
#include <cstdint>
#include <fstream>

#include "../../magic_number.h"
#include "logger.h"

// Length at beginning of packet that is  useless.
// (Due to MAC header, right now.)
constexpr size_t useless_length = 32;
constexpr size_t max_buf_size = 3;
constexpr std::chrono::microseconds duration(packet_samples * 1000000ull /
                                             samples_per_s);

constexpr size_t packet_size = byte_depth * num_channels * packet_samples +
                               useless_length + magic_number.size() +
                               uid.size() + 4 /*packet number*/
                               + 4 /*FCS*/;

typedef std::array<uint8_t, byte_depth> mono_sample_t;
typedef std::array<mono_sample_t, num_channels> sample_t;

extern std::ofstream logfile;

void write_samples(void const *samples, size_t len);
void receive_callback(uint8_t const packet[], size_t size);

void initialize_player();
void playing_loop(std::chrono::time_point<std::chrono::steady_clock>);

template <class logtype>
void log(logtype const &log_m) {
	binary_log(logfile, log_m);
}

#endif /* SOUNDREX_RECEIVE_H */
