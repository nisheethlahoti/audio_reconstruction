#ifndef SOUNDREX_RECEIVE_H
#define SOUNDREX_RECEIVE_H

#include <atomic>
#include <cstdint>
#include "../magic_number.h"

constexpr size_t useless_length = 32; // Length at beginning of packet that is useless. (Due to MAC header, right now.)

constexpr size_t packet_size = byte_depth * num_channels * packet_samples
	+ useless_length
	+ magic_number.size() + uid.size() + 4 /*packet number*/
	+ 4 /*FCS*/;

struct sample_t { // The packet should directly be an array of sample_t type, after the headers. Define as uint8_t[] array, if need be.
	int16_t left, right;
};

enum class packet_result_type {
	invalid_size,
	invalid_magic_number,
	invalid_crc,
	invalid_uid,
	older_packet,
	last_packet,
	full_buffer,
	success,
	reader_waiting,
	packet_recovered_and_written,
	packet_recovered_but_dropped,
	packet_not_recovered,
	majority_not_found,
	not_enough_packets,
	hard_throwaway
};

struct packet_result_t {
	packet_result_type type;
	uint32_t num1, num2; // Any numbers to be given as additional data
	void log();
};

extern std::atomic<int> buf_size;

void write_samples(void const *samples, size_t len);
void receive_callback(uint8_t const packet[], size_t size);

#endif /* SOUNDREX_RECEIVE_H */
