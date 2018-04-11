#include <soundrex/platform_callbacks.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include "logger.h"

inline double sq(double val) { return val * val; }

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

static logger_t packetlogger("packet.bin"), playlogger("play.bin");

template <class log_t>
void packet_log(log_t log) {
	packetlogger.log(log);
}

template <class log_t>
void play_log(log_t log) {
	playlogger.log(log);
}

static constexpr size_t sample_reduce = 4;
static constexpr size_t reduced_samples = packet_samples / sample_reduce;
static std::array<int32_t, reduced_samples * 3> prev_buf{};
static std::array<int32_t, packet_samples * 3> prev_samples{};
static std::array<int32_t, prev_buf.size()> prev_sums{};
static std::array<int64_t, prev_buf.size()> sq_sums{};
static size_t buf_pos = 0;

static inline size_t get_pos(ssize_t val) {
	return (buf_pos + static_cast<size_t>(val + prev_buf.size())) % prev_buf.size();
}

void write_samples(sample_t const *samples, size_t len) {
	assert(num_channels == 1 && packet_samples % sample_reduce == 0);
	assert(len % sample_reduce == 0);
	buf_write(samples, len * sizeof(sample_t));
	for (unsigned i = 0; i < len / sample_reduce; ++i) {
		prev_buf[buf_pos] = 0;
		for (unsigned j = 0; j < sample_reduce; ++j) {
			auto sample = get_int_sample(samples[sample_reduce * i + j][0]);
			prev_buf[buf_pos] += sample;
			prev_samples[sample_reduce * buf_pos + j] = sample;
		}
		prev_sums[buf_pos] = prev_sums[get_pos(-1)] + prev_buf[buf_pos] -
		                     prev_buf[get_pos(-ssize_t(reduced_samples))];
		sq_sums[buf_pos] = sq_sums[get_pos(-1)] + int64_t(prev_buf[buf_pos]) * prev_buf[buf_pos] -
		                   int64_t(prev_buf[get_pos(-ssize_t(reduced_samples))]) *
		                       prev_buf[get_pos(-ssize_t(reduced_samples))];
		buf_pos = get_pos(1);
	}
}

void smooth_merge(packet_t const *const first, packet_t const *const second) {
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
	write_samples(second->samples.data() + samples.size(), second->samples.size() - samples.size());
}

void sharp_merge(packet_t const *, packet_t const *const second) {
	write_samples(second->samples.data(), second->samples.size());
}

void silence(packet_t const *const first, packet_t const *const second) {
	if (first->num == second->num) {
		decltype(first->samples) samples{};
		write_samples(samples.data(), samples.size());
	} else
		write_samples(second->samples.data(), second->samples.size());
}

void white_noise(packet_t const *const first, packet_t const *const second) {
	static std::default_random_engine gen(12345);
	if (first->num == second->num) {
		decltype(first->samples) samples;
		for (auto &sample : samples)
			for (auto &mono : sample)
				for (uint8_t &byte : mono)
					byte = gen() & 0xff;
		write_samples(samples.data(), samples.size());
	} else
		write_samples(second->samples.data(), second->samples.size());
}

void reconstruct(packet_t const *const first, packet_t const *const second) {
	std::default_random_engine gen(54321);
	std::bernoulli_distribution dist(0.2);
	double best_correl = 0;
	ssize_t best_diff = reduced_samples;
	int64_t signum = 1;
	for (ssize_t diff = reduced_samples; diff < ssize_t(2 * reduced_samples); ++diff) {
		int64_t inner = 0;
		for (ssize_t i = 0; i < ssize_t(reduced_samples); ++i)
			inner += int64_t(prev_buf[get_pos(-i-1)]) * prev_buf[get_pos(-diff - i-1)];
		inner -= int64_t(prev_sums[get_pos(-1)]) * prev_sums[get_pos(-diff-1)] / ssize_t(reduced_samples);
	double correl =
	    double(inner)  / std::sqrt((sq_sums[get_pos(-1)] - sq(prev_sums[get_pos(-1)])/reduced_samples) *
	                          (sq_sums[get_pos(-diff-1)] - sq(prev_sums[get_pos(-diff-1)])/reduced_samples));
		if (std::abs(correl) > best_correl) {
			signum = inner >= 0 ? 1 : -1;
			best_diff = diff;
			best_correl = std::abs(correl);
		}
	}
	// if (dist(gen))
	//std::clog << best_correl << " correlate with " << (sq_sums[buf_pos] >> 25) << "       \r"
	 //         << std::flush;
	decltype(second->samples) samples;
	for (size_t i = 0; i < samples.size(); ++i) {
		put_int_sample(
		    (std::min(
		        int64_t(std::numeric_limits<int32_t>::max()),
		        std::max(int64_t(std::numeric_limits<int32_t>::min()),
		                 (int64_t(sample_reduce *
		                          prev_samples[sample_reduce *
		                                           get_pos(ssize_t(i / sample_reduce) - best_diff) +
		                                       i % sample_reduce]) -
		                  prev_sums[get_pos(-best_diff-1)] / ssize_t(reduced_samples)) *
		                         signum +
		                     prev_sums[get_pos(-1)] / ssize_t(reduced_samples)))) /
		        sample_reduce,
		    samples[i][0]);
	}
	/*for (int i = 0; i < 30; ++i) {
	    int32_t level = prev_samples[(buf_pos + prev_samples.size() - 30 + i) %
	prev_samples.size()]; int64_t const val = get_int_sample(samples[i][0]); put_int_sample(level +
	i * (val - level) / 30, samples[i][0]);
	}*/
	for (int i = 0; i < ssize_t(trailing_samples / 2); ++i) {
		sample_t &smp = samples[i];
		sample_t const &s2 = samples[i], &s1 = packets.front().samples[i];
		for (unsigned ch = 0; ch < smp.size(); ++ch) {
			int64_t const v1 = get_int_sample(s1[ch]), v2 = get_int_sample(s2[ch]);
			put_int_sample(v1 + i * (v2 - v1) / ssize_t(trailing_samples / 2), smp[ch]);
		}
		sample_t &smpx = samples[packet_samples - 1 - i];
		sample_t const &s4 = samples[packet_samples - 1 - i],
		               &s3 = packets.front().samples[packet_samples - 1 - i];
		for (unsigned ch = 0; ch < smp.size(); ++ch) {
			int64_t const v1 = get_int_sample(s3[ch]), v2 = get_int_sample(s4[ch]);
			put_int_sample(v1 + i * (v2 - v1) / ssize_t(trailing_samples / 2), smpx[ch]);
		}
	}
	for (unsigned i = 0; i < packet_samples; ++i) {
		sample_t &smp = samples[i];
		sample_t const &s1 = samples[i], &s2 = packets.front().samples[i];
		for (unsigned ch = 0; ch < smp.size(); ++ch) {
			int64_t const v1 = get_int_sample(s1[ch]), v2 = get_int_sample(s2[ch]);
			if (best_correl > 0.95)
			put_int_sample((v1 +  v2) / 2, smp[ch]);
			else
			put_int_sample((v1 + 2 * v2) / 3, smp[ch]);
		}
	}

	if (first->num == second->num) {
		write_samples(samples.data(), samples.size());
	} else {
		for (int i = 0; i < ssize_t(trailing_samples); ++i) {
			sample_t &smp = samples[i];
			sample_t const &s1 = samples[i], &s2 = second->samples[i];
			for (unsigned ch = 0; ch < smp.size(); ++ch) {
				int64_t const v1 = get_int_sample(s1[ch]), v2 = get_int_sample(s2[ch]);
				put_int_sample(v1 + i * (v2 - v1) / ssize_t(trailing_samples), smp[ch]);
			}
		}
		write_samples(samples.data(), trailing_samples);
		write_samples(second->samples.data() + trailing_samples,
		              second->samples.size() - trailing_samples);
	}
}

#define LOG_TYPE(_, NAME, ...)                                        \
	template void packet_log<CONCAT(NAME, _log)>(CONCAT(NAME, _log)); \
	template void play_log<CONCAT(NAME, _log)>(CONCAT(NAME, _log));

#include <soundrex/logtypes.list>
