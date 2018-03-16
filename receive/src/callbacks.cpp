#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>

#include "receive.h"

using namespace std;
typedef array<uint8_t, packet_size> packet_t;

static uint32_t latest_packet_number = 0;
atomic<bool> correction_on(true);

static array<batch_t, max_buf_size + 1> batches;
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

static inline bool write_packet(uint8_t const *packet, uint32_t pnum, logger_t &logger) {
	b_itr const finish = b_end.load(memory_order_relaxed);
	if (b_begin.load(memory_order_consume) == finish) {
		logger.log(full_buffer_log(pnum));
		return false;
	} else {
		finish->num = pnum;
		memcpy(finish->samples.data(), packet, sizeof finish->samples);
		memcpy(finish->trailing.data(), packet + sizeof finish->samples, sizeof finish->trailing);
		b_end.store(finish == batches.end() - 1 ? batches.begin() : finish + 1,
		            memory_order_release);
		logger.log(success_log(pnum));
		return true;
	}
}

bool receive_callback(raw_packet_t packet, logger_t &logger) {
	uint8_t const *startpos = packet.data + useless_length;
	if (packet.size != packet_size) {
		logger.log(invalid_size_log(static_cast<uint16_t>(packet.size)));
		return false;
	}

	array<uint8_t, 4> xor_val = magic_number;
	for (int i = 0; i < 4; ++i) {
		xor_val[i] ^= startpos[i] ^ startpos[i + 4] ^ startpos[i + 8];
	}

	if (xor_val != array<uint8_t, 4>()) {
		logger.log(invalid_magic_number_log(get_little_endian(startpos)));
		return false;
	}

	if (!equal(uid.begin(), uid.end(), startpos)) {
		logger.log(invalid_uid_log(get_little_endian(startpos)));
		return false;
	}

	uint32_t packet_number = get_little_endian(uid.size() + startpos);
	if (packet_number < latest_packet_number) {
		logger.log(older_packet_log(latest_packet_number, packet_number));
		return true;
	}

	if (packet_number == latest_packet_number) {
		logger.log(repeated_packet_log(packet_number));
		return true;
	}

	latest_packet_number = packet_number;
	logger.log(validated_log());
	write_packet(startpos + 12, packet_number, logger);
	return true;
}

inline static int32_t get_int_sample(mono_sample_t const &smpl) {
	int32_t ret;
	memcpy(reinterpret_cast<uint8_t *>(&ret) + (4 - sizeof smpl), &smpl, sizeof smpl);
	return ret >> (8 * (4 - sizeof smpl));
}

static void mergewrite_samples(b_const_itr const first, b_const_itr const second) {
	if (!correction_on.load(memory_order_consume) || second->num == first->num + 1) {
		write_samples(second->samples.data(), second->samples.size());
	} else {
		array<sample_t, batch_t().trailing.size()> samples;
		for (int i = 0; i < samples.size(); ++i) {
			sample_t &smp = samples[i];
			sample_t const &s1 = first->trailing[i], &s2 = second->samples[i];
			for (int ch = 0; ch < smp.size(); ++ch) {
				int64_t const v1 = get_int_sample(s1[ch]), v2 = get_int_sample(s2[ch]);
				int32_t val = v1 + i * (v2 - v1) / ssize_t(samples.size());
				memcpy(&smp[ch], &val, sizeof smp[ch]);
			}
		}
		write_samples(samples.data(), samples.size());
		write_samples(second->samples.data() + samples.size(),
		              second->samples.size() - samples.size());
	}
}

void play_next(logger_t &logger) {
	static int further_repeat = 0;
	b_itr const start = b_begin.load(memory_order_relaxed);
	b_itr const next = start == batches.end() - 1 ? batches.begin() : start + 1;

	if (b_end.load(memory_order_acquire) != next) {
		further_repeat = max_repeat;
		mergewrite_samples(start, next);
		b_begin.store(next, memory_order_release);
		logger.log(playing_log(next->num));
	} else if (further_repeat) {
		--further_repeat;
		mergewrite_samples(start, start);
		logger.log(repeat_play_log(start->num));
	} else {
		static constexpr array<sample_t, packet_samples> zeroes{};
		logger.log(reader_waiting_log());
		write_samples(zeroes.data(), zeroes.size());
	}
}
