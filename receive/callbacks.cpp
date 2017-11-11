#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "receive.h"
#include "../crc.h"

using namespace std;
static constexpr size_t max_buf_size = 6;

static uint32_t latest_packet_number = 0;

typedef array<uint8_t, packet_size> packet_t;

static packet_t packet_versions[max_redundancy];
static unsigned int num_versions = 0;
static bool validated = true, written = true;

static inline uint32_t get_little_endian(uint8_t const *bytes) {
	uint32_t ret = 0;
	for (int i=0; i<4; ++i) {
		ret = ret | bytes[i] << (8*i);
	}
	return ret;
}

static inline bool write_packet(uint8_t const *packet) {
	if (buf_size >= max_buf_size) {
		return false;
	}

	buf_size++;
	write_samples(packet, packet_samples);
	return true;
}

static inline packet_result_type check_majority() { // TODO: Test if "more efficient" algorithm is indeed more efficient, and if so, implement.
	for (int i=0; i<packet_versions[0].size(); ++i) {
		int match = 0, j;
		for (j=0; j<num_versions-1 && match < num_versions/2; ++j) {
			for (int k=j+1; k<num_versions; ++k) {
				if (packet_versions[k][i] == packet_versions[j][i])
					match++;
			}
		}
		if (match == num_versions/2)
			packet_versions[0][i] = packet_versions[j][i];
		else
			return packet_result_type::majority_not_found;
	}

	uint8_t *packet = packet_versions[0].data();
	uint32_t expected_crc = crc32(packet, packet_size-4), received_crc = get_little_endian(packet+packet_size-4);
	if (expected_crc == received_crc) {
		return write_packet(packet + useless_length + 12)
			? packet_result_type::packet_recovered_and_written
			: packet_result_type::packet_recovered_but_dropped;
	} else {
		return packet_result_type::packet_not_recovered;
	}
}

static inline packet_result_t receive_unlogged(uint8_t const *packet, size_t size) {
	uint8_t const *startpos = packet + useless_length;
	if (size != packet_size) {
		return {packet_result_type::invalid_size, static_cast<uint32_t>(size)};
	}

	array<uint8_t, 4> xor_val = magic_number;
	for (int i=0; i<4; ++i) {
		xor_val[i] ^= startpos[i] ^ startpos[i+4] ^ startpos[i+8];
	}
	if (xor_val != array<uint8_t, 4>()) {
		return {packet_result_type::invalid_magic_number, get_little_endian(startpos)};
	}

	if (!equal(uid.begin(), uid.end(), startpos)) {
		return {packet_result_type::invalid_uid, get_little_endian(startpos)};
	}

	uint32_t packet_number = get_little_endian(uid.size() + startpos);
	if (packet_number < latest_packet_number) {
		return {packet_result_type::older_packet, packet_number, latest_packet_number};
	}

	if (packet_number == latest_packet_number && validated) {
		return {packet_result_type::last_packet, packet_number};
	}

	if (packet_number > latest_packet_number) {
		if (!written) {
			if (validated) {
				if (write_packet(packet_versions[0].data() + useless_length + 12)) { // Skipping ahead of packet_number and the XOR
					packet_result_t({packet_result_type::success, latest_packet_number}).log();
				} else {
					packet_result_t({packet_result_type::hard_throwaway, latest_packet_number}).log();
				}
			} else if (num_versions > 2) {
				packet_result_t({check_majority(), latest_packet_number, num_versions}).log();
			} else {
				packet_result_t({packet_result_type::not_enough_packets, latest_packet_number, num_versions}).log();
			}
		}
		latest_packet_number = packet_number;
		num_versions = 0;
		validated = false;
		written = false;
	}

	uint32_t expected_crc = crc32(packet, size-4), received_crc = get_little_endian(packet+size-4);
	if (expected_crc != received_crc) {
		copy(packet, packet + size, packet_versions[num_versions++].data());
		return {packet_result_type::invalid_crc, received_crc, expected_crc};
	}

	validated = true;
	if (write_packet(startpos + 12)) { // Skipping ahead of packet_number and the XOR
		written = true;
		return {packet_result_type::success, packet_number};
	} else {
		copy(packet, packet + size, packet_versions[0].data());
		return {packet_result_type::full_buffer, packet_number}; // TODO: Just dump into the packet_versions array, to be checked on arrival of next packet.
	}
}

void receive_callback(uint8_t const *packet, size_t size) {
	receive_unlogged(packet, size).log();
}

