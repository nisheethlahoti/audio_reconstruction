#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>

#include <soundrex/lib/processor.hpp>
#include <soundrex/platform_callbacks.hpp>

using namespace std;

static constexpr chrono::milliseconds max_play_at_end(50);
static constexpr int max_repeat = max_play_at_end / duration;

static inline uint32_t get_little_endian(uint8_t const *bytes) {
	return bytes[0] | uint32_t(bytes[1]) << 8 | uint32_t(bytes[2]) << 16 | uint32_t(bytes[3]) << 24;
}

inline static int32_t get_int_sample(mono_sample_t const &smpl) {
	int32_t ret = 0;
	for (unsigned i = 0; i < sizeof(smpl); ++i)
		ret |= uint32_t(smpl[i]) << (8 * (4 - sizeof(smpl) + i));
	return ret >> (8 * (4 - sizeof smpl));
}

inline static void put_int_sample(int32_t val, mono_sample_t &smpl) {
	for (unsigned i = 0; i < sizeof(smpl); ++i)
		smpl[i] = (val >> (8 * i)) & 0xff;
}

void processor_t::process(uint8_t const *packet) {
	uint32_t packet_number = get_little_endian(packet);
	if (packet_number < latest_packet_number) {
		packet_log(older_packet_log(latest_packet_number, packet_number));
	} else if (packet_number == latest_packet_number) {
		packet_log(repeated_packet_log(packet_number));
	} else if (b_itr const finish = b_end.load(memory_order_relaxed);
	           b_begin.load(memory_order_consume) == finish) {
		packet_log(full_buffer_log(packet_number));
	} else {
		latest_packet_number = packet_number;
		memcpy(finish, packet, sizeof(packet_t));
		finish->num = packet_number;
		b_end.store(finish == batches.end() - 1 ? batches.begin() : finish + 1,
		            memory_order_release);
		packet_log(success_log(packet_number));
	}
}

void processor_t::mergewrite_samples(b_const_itr const first, b_const_itr const second) const {
	if (!correction_on.load(memory_order_consume) || second->num == first->num + 1) {
		write_samples(second->samples.data(), second->samples.size());
	} else {
		decltype(first->trailing) samples;
		for (unsigned i = 0; i < samples.size(); ++i) {
			sample_t &smp = samples[i];
			sample_t const &s1 = first->trailing[i], &s2 = second->samples[i];
			for (unsigned ch = 0; ch < smp.size(); ++ch) {
				int64_t const v1 = get_int_sample(s1[ch]), v2 = get_int_sample(s2[ch]);
				put_int_sample(v1 + i * (v2 - v1) / ssize_t(samples.size()), smp[ch]);
			}
		}
		write_samples(samples.data(), samples.size());
		write_samples(second->samples.data() + samples.size(),
		              second->samples.size() - samples.size());
	}
}

void processor_t::play_next() {
	static int further_repeat = 0;
	b_itr const start = b_begin.load(memory_order_relaxed);
	b_itr const next = start == batches.end() - 1 ? batches.begin() : start + 1;

	if (b_end.load(memory_order_acquire) != next) {
		further_repeat = max_repeat;
		mergewrite_samples(start, next);
		b_begin.store(next, memory_order_release);
		play_log(playing_log(next->num));
	} else if (further_repeat) {
		--further_repeat;
		mergewrite_samples(start, start);
		play_log(repeat_play_log(start->num));
	} else {
		static constexpr array<sample_t, packet_samples> zeroes{};
		play_log(reader_waiting_log());
		write_samples(zeroes.data(), zeroes.size());
	}
}

bool processor_t::toggle_corrections() {
	bool corr = correction_on.load(std::memory_order_relaxed);
	correction_on.store(!corr, std::memory_order_release);
	return !corr;
}
