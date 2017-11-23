#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

#include "receive.h"

using namespace std;
typedef array<uint8_t, packet_size> packet_t;

static uint32_t latest_packet_number = 0;
static packet_t packet_versions[max_redundancy];
static unsigned int num_versions = 0;
static bool validated = true, written = true;

array<batch_t, max_buf_size + 1> batches;
typedef decltype(batches)::iterator b_itr;
typedef decltype(batches)::const_iterator b_const_itr;
static b_itr b_start = batches.begin(), b_end = batches.begin() + 1;
static mutex batch_mut;

static uint32_t const crctable[] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL};

static uint32_t crc32(const uint8_t *bytes, size_t bytes_sz) {
	uint32_t crc = ~0;
	for (size_t i = 0; i < bytes_sz; ++i) {
		crc = crctable[(crc ^ bytes[i]) & 0xff] ^ (crc >> 8);
	}
	return ~crc;
}

static inline uint32_t get_little_endian(uint8_t const *bytes) {
	uint32_t ret = 0;
	for (int i = 0; i < 4; ++i) {
		ret = ret | bytes[i] << (8 * i);
	}
	return ret;
}

static inline bool write_packet(uint8_t const *packet, uint32_t const pnum) {
	batch_mut.lock();
	if (b_start == b_end) {
		batch_mut.unlock();
		log(full_buffer_log(pnum));
		return false;
	} else {
		b_end->num = pnum;
		memcpy(b_end->samples.data(), packet, sizeof b_end->samples);
		memcpy(b_end->trailing.data(), packet + sizeof b_end->samples,
		       sizeof b_end->trailing);
		b_end = b_end == batches.end() - 1 ? batches.begin() : b_end + 1;
		batch_mut.unlock();
		log(success_log(pnum));
		return true;
	}
}

static inline void check_majority() {
	// TODO: Test if "more efficient" algorithm is indeed more efficient, and if
	// so, implement.
	for (int i = 0; i < packet_versions[0].size(); ++i) {
		int match = 0, j;
		for (j = 0; j < num_versions - 1 && match < num_versions / 2; ++j) {
			for (int k = j + 1; k < num_versions; ++k) {
				if (packet_versions[k][i] == packet_versions[j][i])
					match++;
			}
		}
		if (match == num_versions / 2)
			packet_versions[0][i] = packet_versions[j][i];
		else {
			log(majority_not_found_log(latest_packet_number, num_versions));
			return;
		}
	}

	uint8_t *packet = packet_versions[0].data();
	uint32_t expected_crc = crc32(packet, packet_size - 4),
	         received_crc = get_little_endian(packet + packet_size - 4);
	if (expected_crc == received_crc) {
		log(packet_recovered_log(latest_packet_number, num_versions));
		write_packet(packet + useless_length + 12, latest_packet_number);
	} else {
		log(packet_not_recovered_log(latest_packet_number, num_versions));
	}
}

void receive_callback(uint8_t const *packet, size_t size) {
	static mutex mut;
	lock_guard<mutex> lock(mut);

	uint8_t const *startpos = packet + useless_length;
	if (size != packet_size) {
		log(invalid_size_log(static_cast<uint16_t>(size)));
		return;
	}

	array<uint8_t, 4> xor_val = magic_number;
	for (int i = 0; i < 4; ++i) {
		xor_val[i] ^= startpos[i] ^ startpos[i + 4] ^ startpos[i + 8];
	}

	if (xor_val != array<uint8_t, 4>()) {
		log(invalid_magic_number_log(get_little_endian(startpos)));
		return;
	}

	if (!equal(uid.begin(), uid.end(), startpos)) {
		log(invalid_uid_log(get_little_endian(startpos)));
		return;
	}

	uint32_t packet_number = get_little_endian(uid.size() + startpos);
	if (packet_number < latest_packet_number) {
		log(older_packet_log(latest_packet_number, packet_number));
		return;
	}

	if (packet_number == latest_packet_number && validated) {
		log(repeated_packet_log(packet_number));
		return;
	}

	if (packet_number > latest_packet_number) {
		if (!written) {
			if (validated) {
				log(deferred_writing_log());
				write_packet(packet_versions[0].data() + useless_length + 12,
				             latest_packet_number);
			} else if (num_versions > 2) {
				check_majority();
			} else {
				log(not_enough_packets_log(latest_packet_number, num_versions));
			}
		}
		latest_packet_number = packet_number;
		num_versions = 0;
		validated = false;
		written = false;
	}

	uint32_t expected_crc = crc32(packet, size - 4),
	         received_crc = get_little_endian(packet + size - 4);
	if (expected_crc != received_crc) {
		copy(packet, packet + size, packet_versions[num_versions++].data());
		log(invalid_crc_log(expected_crc, received_crc));
		return;
	}

	validated = true;
	log(validated_log());
	if (write_packet(startpos + 12, packet_number)) {
		written = true;
	} else {
		copy(packet, packet + size, packet_versions[0].data());
	}
}

inline static int32_t get_int_sample(mono_sample_t const &smpl) {
	int32_t ret;
	memcpy(reinterpret_cast<uint8_t *>(&ret) + 4 - sizeof smpl, &smpl,
	       sizeof smpl);
	return ret >> (8 * (4 - sizeof smpl));
}

inline void mergewrite_samples(b_const_itr first, b_const_itr second) {
	if (second->num == first->num + 1) {
		write_samples(second->samples.data(), second->samples.size());
	} else {
		array<sample_t, batch_t().trailing.size()> samples;
		for (int i = 0; i < samples.size(); ++i) {
			sample_t &smp = samples[i];
			sample_t const &s1 = first->trailing[i], &s2 = second->samples[i];
			for (int ch = 0; ch < smp.size(); ++ch) {
				int64_t const v1 = get_int_sample(s1[ch]),
				              v2 = get_int_sample(s2[ch]);
				int32_t val = v1 + i * (v2 - v1) / samples.size();
				memcpy(&smp[ch], &val, sizeof smp[ch]);
			}
		}
		write_samples(samples.data(), samples.size());
		write_samples(second->samples.data() + samples.size(),
		              second->samples.size() - samples.size());
	}
}

void playing_loop(chrono::time_point<chrono::steady_clock> time) {
	static int further_repeat = 0;
	while (true) {
		this_thread::sleep_until(time += duration);
		batch_mut.lock();
		b_itr const b_next =
		    b_start == batches.end() - 1 ? batches.begin() : b_start + 1;
		if (b_end != b_next) {
			further_repeat = max_repeat;
			mergewrite_samples(b_start, b_next);
			b_start = b_next;
			batch_mut.unlock();
			log(playing_log(b_start->num));
		} else if (further_repeat) {
			--further_repeat;
			mergewrite_samples(b_start, b_start);
			batch_mut.unlock();
			log(repeat_play_log(b_start->num));
		} else {
			batch_mut.unlock();
			log(reader_waiting_log());
			int bdiff = 0;
			do {
				this_thread::sleep_until(time += duration);
				batch_mut.lock();
				bdiff = b_end - b_next;
				batch_mut.unlock();
				if (bdiff < 0)
					bdiff += batches.size();
			} while (bdiff < batches.size() / 2);
		}
	}
}

uint32_t get_timediff() {
	static mutex mut;
	static auto tp = chrono::steady_clock::now();
	auto prev = tp;
	tp = chrono::steady_clock::now();
	auto val = chrono::duration_cast<chrono::microseconds>(tp - prev).count();
	return val >> 31 ? ~0 : val;
}
