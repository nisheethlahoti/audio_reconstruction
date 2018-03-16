#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>

#include <receiver/receiver.h>
#include <receiver/platform_callbacks.h>

using namespace std;

static inline uint32_t get_little_endian(uint8_t const *bytes) {
	return bytes[0] | bytes[1] << 8 | bytes[2] << 16 | bytes[3] << 24;
}

inline static int32_t get_int_sample(mono_sample_t const &smpl) {
	int32_t ret;
	memcpy(reinterpret_cast<uint8_t *>(&ret) + (4 - sizeof smpl), &smpl, sizeof smpl);
	return ret >> (8 * (4 - sizeof smpl));
}

bool receiver_t::write_packet(uint8_t const *packet, uint32_t pnum) {
	b_itr const finish = b_end.load(memory_order_relaxed);
	if (b_begin.load(memory_order_consume) == finish) {
		packet_log(full_buffer_log(pnum));
		return false;
	} else {
		finish->num = pnum;
		memcpy(finish->samples.data(), packet, sizeof finish->samples);
		memcpy(finish->trailing.data(), packet + sizeof finish->samples, sizeof finish->trailing);
		b_end.store(finish == batches.end() - 1 ? batches.begin() : finish + 1,
		            memory_order_release);
		packet_log(success_log(pnum));
		return true;
	}
}

bool receiver_t::receive_callback(raw_packet_t const packet) {
	if (packet.size != packet_size) {
		packet_log(invalid_size_log(packet.size));
		return false;
	}

	array<uint8_t, 4> xor_val = magic_number;
	for (int i = 0; i < 4; ++i) {
		xor_val[i] ^= packet.data[i] ^ packet.data[i + 4] ^ packet.data[i + 8];
	}

	if (xor_val != array<uint8_t, 4>()) {
		packet_log(invalid_magic_number_log(get_little_endian(packet.data)));
		return false;
	}

	if (!equal(uid.begin(), uid.end(), packet.data)) {
		packet_log(invalid_uid_log(get_little_endian(packet.data)));
		return false;
	}

	uint32_t packet_number = get_little_endian(uid.size() + packet.data);
	if (packet_number < latest_packet_number) {
		packet_log(older_packet_log(latest_packet_number, packet_number));
		return true;
	}

	if (packet_number == latest_packet_number) {
		packet_log(repeated_packet_log(packet_number));
		return true;
	}

	latest_packet_number = packet_number;
	packet_log(validated_log());
	write_packet(packet.data + 12, packet_number);
	return true;
}

void receiver_t::mergewrite_samples(b_const_itr const first, b_const_itr const second) {
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

void receiver_t::play_next() {
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

bool receiver_t::toggle_corrections() {
	bool corr = correction_on.load(std::memory_order_relaxed);
	correction_on.store(!corr, std::memory_order_release);
	return !corr;
}
