#include <soundrex/lib/processor.h>
#include <soundrex/platform_callbacks.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <iostream>

using namespace std;

static constexpr chrono::milliseconds max_play_at_end(50);
static constexpr int max_repeat = max_play_at_end / duration;

static inline uint32_t get_little_endian(uint8_t const *bytes) {
	return bytes[0] | uint32_t(bytes[1]) << 8 | uint32_t(bytes[2]) << 16 | uint32_t(bytes[3]) << 24;
}

void processor_t::process(uint8_t const *packet) {
	uint32_t packet_number = get_little_endian(packet + 4);
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

void mergewrite_samples(reconstruct_t recon, packet_t const *first, packet_t const *second) {
	if (second->num == first->num + 1) {
		write_samples(second->samples.data(), second->samples.size());
	} else
		switch (recon) {
			case reconstruct_t::smooth_merge:
				smooth_merge(first, second);
				break;
			case reconstruct_t::sharp_merge:
				sharp_merge(first, second);
				break;
			case reconstruct_t::silence:
				silence(first, second);
				break;
			case reconstruct_t::white_noise:
				white_noise(first, second);
				break;
			case reconstruct_t::reconstruct:
				reconstruct(first, second);
				break;
		}
}

void processor_t::play_next() {
	static int further_repeat = 0, diff_ok = 1;
	b_itr const start = b_begin.load(memory_order_relaxed);
	b_itr const next = start == batches.end() - 1 ? batches.begin() : start + 1;

	reconstruct_t correct = reconstruct.load(memory_order_acquire);
	nominal = start->num + diff_ok;
	if (packets.empty()) {
		std::cerr << "Whaaa\n";
	} else
		while (packets.front().num != nominal) {
			packets.pop();
			if (packets.empty()) {
				std::cerr << "Whaaa\n";
				break;
			}
		}
	if (b_end.load(memory_order_acquire) != next) {
		further_repeat = max_repeat;
		if (next->num <= start->num + diff_ok || next->num > start->num + max_buf_size) {
			mergewrite_samples(correct, start, next);
			b_begin.store(next, memory_order_release);
			diff_ok = 1;
			play_log(playing_log(next->num));
		} else {
			mergewrite_samples(correct, start, start);
			play_log(repeat_play_log(start->num));
			diff_ok++;
		}
	} else if (further_repeat) {
		--further_repeat;
		mergewrite_samples(correct, start, start);
		play_log(repeat_play_log(start->num));
		diff_ok++;
	} else {
		static constexpr array<sample_t, packet_samples> zeroes{};
		play_log(reader_waiting_log());
		write_samples(zeroes.data(), zeroes.size());
	}
}
