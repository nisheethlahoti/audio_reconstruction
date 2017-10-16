#ifndef SOUNDREX_RECEIVE_H
#define SOUNDREX_RECEIVE_H

#include <cstdint>
#include "../magic_number.h"

constexpr size_t useless_length = 32; // Length at beginning of packet that is useless. (Due to MAC header, right now.)

constexpr size_t packet_size = byte_depth * num_channels * packet_samples
	+ useless_length
	+ magic_number.size() + uid.size() + 4 /*packet number*/
	+ 4 /*FCS*/;

struct sample_t { // The packet should directly be an array of sample_t type, after the headers. Define as uint8_t[] array, if need be.
	uint16_t left_low;
	int8_t left_high;
	uint8_t right_low;
	int16_t right_high;
};

enum class packet_result_type {
	invalid_size,
	invalid_magic_number,
	invalid_crc,
	invalid_uid,
	older_packet,
	full_buffer,
	success
};

struct packet_result_t {
	packet_result_type type;
	uint32_t num1, num2; // Any numbers to be given as additional data
	void log();
};

void receive_callback(uint8_t packet[], size_t size);
void timer_callback();
void send_sample(sample_t sample);

#endif /* SOUNDREX_RECEIVE_H */
