#ifndef SOUNDREX_RECEIVE_H
#define SOUNDREX_RECEIVE_H

#include <atomic>
#include <cstdint>
#include <fstream>

#include "../../magic_number.h"
#include "logger.h"

// Length at beginning of packet that is  useless.
// (Due to MAC header, right now.)
constexpr size_t useless_length = 32;
static constexpr size_t max_buf_size = 3;

constexpr size_t packet_size = byte_depth * num_channels * packet_samples +
                               useless_length + magic_number.size() +
                               uid.size() + 4 /*packet number*/
                               + 4 /*FCS*/;

struct sample_t {
	// The packet should directly be an array of sample_t type, after the
	// headers. Define as uint8_t[] array, if need be.
	int16_t left, right;
};

extern std::atomic<int> buf_size;
extern std::ofstream logfile;

void write_samples(void const *samples, size_t len);
void receive_callback(uint8_t const packet[], size_t size);

template <class logtype>
void log(logtype const &log_m) {
	binary_log(logfile, log_m);
}

#endif /* SOUNDREX_RECEIVE_H */
