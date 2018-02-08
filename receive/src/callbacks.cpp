#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <thread>

#include "receive.h"

using namespace std;

static uint32_t latest_packet_number = 0;

static inline uint32_t get_little_endian(uint8_t const *bytes) {
	uint32_t ret = 0;
	for (int i = 0; i < 4; ++i) {
		ret = ret | bytes[i] << (8 * i);
	}
	return ret;
}

void receive_callback(raw_packet_t packet, logger_t &logger) {
	uint8_t const *startpos = packet.data + useless_length;
	if (packet.size != packet_size && packet.size != packet_size + 4) {
		logger.log(invalid_size_log(static_cast<uint16_t>(packet.size)));
		return;
	}

	array<uint8_t, 4> xor_val = magic_number;
	for (int i = 0; i < 4; ++i) {
		xor_val[i] ^= startpos[i] ^ startpos[i + 4] ^ startpos[i + 8];
	}

	if (xor_val != array<uint8_t, 4>()) {
		logger.log(invalid_magic_number_log(get_little_endian(startpos)));
		return;
	}

	if (!equal(uid.begin(), uid.end(), startpos)) {
		logger.log(invalid_uid_log(get_little_endian(startpos)));
		return;
	}

	uint32_t packet_number = get_little_endian(uid.size() + startpos);
	if (packet_number < latest_packet_number) {
		logger.log(older_packet_log(latest_packet_number, packet_number));
		return;
	}

	if (packet_number == latest_packet_number) {
		logger.log(repeated_packet_log(packet_number));
		return;
	}

	latest_packet_number = packet_number;
	logger.log(validated_log(packet.size));
	logger.write(packet.data, packet.size);
}
