#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <iomanip>
#include <iostream>
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
static atomic<b_itr> b_begin(batches.begin()), b_end(batches.begin() + 1);

static inline uint32_t get_little_endian(uint8_t const *bytes) {
	uint32_t ret = 0;
	for (int i = 0; i < 4; ++i) {
		ret = ret | bytes[i] << (8 * i);
	}
	return ret;
}

static inline bool write_packet(uint8_t const *packet, uint32_t const pnum) {
	b_itr const finish = b_end.load(memory_order_relaxed);
	if (b_begin.load(memory_order_consume) == finish) {
		log(full_buffer_log(pnum));
		return false;
	} else {
		finish->num = pnum;
		memcpy(finish->samples.data(), packet, sizeof finish->samples);
		memcpy(finish->trailing.data(), packet + sizeof finish->samples,
		       sizeof finish->trailing);
		b_end.store(finish == batches.end() - 1 ? batches.begin() : finish + 1,
		            memory_order_release);
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
	log(packet_recovered_log(latest_packet_number, num_versions));
	write_packet(packet + useless_length + 12, latest_packet_number);
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

static void mergewrite_samples(b_const_itr const first,
                               b_const_itr const second) {
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
				int32_t val = v1 + i * (v2 - v1) / int(samples.size());
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
	static int tick = 0;
	static bool dealt_with_tick = false;
	while (true) {
		this_thread::sleep_until(time += duration);
		b_itr const start = b_begin.load(memory_order_relaxed);
		b_itr const next =
		    start == batches.end() - 1 ? batches.begin() : start + 1;
		if (tick != 0) {
			--tick;
			mergewrite_samples(start, start);
		} else if (b_end.load(memory_order_consume) != next) {
			further_repeat = max_repeat;
			if (false) { //(!dealt_with_tick && next->num > start->num+1) {
				tick = next->num - start->num - 2;
				mergewrite_samples(start, start);
				dealt_with_tick = true;
			} else {
				mergewrite_samples(start, next);
				b_begin.store(next, memory_order_release);
				dealt_with_tick = false;
			}
			log(playing_log(next->num));
		} else if (further_repeat) {
			--further_repeat;
			mergewrite_samples(start, start);
			log(repeat_play_log(start->num));
		} else {
			log(reader_waiting_log());
			int bdiff = 0;
			do {
				this_thread::sleep_until(time += duration);
				bdiff = b_end.load(memory_order_consume) - next;
				if (bdiff < 0)
					bdiff += batches.size();
			} while (bdiff < batches.size() / 2);
		}
	}
}

uint32_t get_timediff() {
	static auto tp = chrono::steady_clock::now();
	auto prev = tp;
	tp = chrono::steady_clock::now();
	auto val = chrono::duration_cast<chrono::microseconds>(tp - prev).count();
	return val >> 31 ? ~0 : val;
}
