#include <cstring>
#include <iostream>

#include "../sockreceive/wav_direct_write.h"
#include "../src/logger.h"
#include "../src/receive.h"

using namespace std;

struct batch_t {
	uint32_t num;
	std::array<sample_t, packet_samples> samples;
	std::array<sample_t, trailing_samples> trailing;
};

constexpr size_t max_buf_size = 8;
constexpr uint64_t inc_micros = duration.count();

array<batch_t, max_buf_size + 1> batches;
typedef decltype(batches)::iterator b_itr;
typedef decltype(batches)::const_iterator b_const_itr;
b_const_itr b_begin(batches.begin());
b_itr b_end(batches.begin() + 1);
uint64_t curr_micros, next_play_event;

wav_header_t header(num_channels, samples_per_s, byte_depth);
wav_writer_t<sample_t> outp("outp.wav", header);

static inline uint32_t get_little_endian(uint8_t const *bytes) {
	uint32_t ret = 0;
	for (int i = 0; i < 4; ++i) {
		ret = ret | bytes[i] << (8 * i);
	}
	return ret;
}

static inline void write_packet(uint8_t const *packet, uint32_t pnum, logger_t &logger) {
	if (b_begin == b_end) {
		logger.log(full_buffer_log(pnum));
	} else {
		b_end->num = pnum;
		memcpy(b_end->samples.data(), packet, sizeof b_end->samples);
		memcpy(b_end->trailing.data(), packet + sizeof b_end->samples, sizeof b_end->trailing);
		b_end = b_end == batches.end() - 1 ? batches.begin() : b_end + 1;
		logger.log(success_log(pnum));
	}
}

inline static int32_t get_int_sample(mono_sample_t const &smpl) {
	int32_t ret;
	memcpy(reinterpret_cast<uint8_t *>(&ret) + (4 - sizeof smpl), &smpl, sizeof smpl);
	return ret >> (8 * (4 - sizeof smpl));
}

void write_samples(void const *samples, size_t len) {
	sample_t const *smpl = static_cast<sample_t const *>(samples);
	while (len--) {
		outp.add_sample(*smpl++);
	}
}

static void mergewrite_samples(b_const_itr const first, b_const_itr const second) {
	if (second->num == first->num + 1) {
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

void playing_loop(logger_t &logger) {
	static int further_repeat = 0;
	b_const_itr const start = b_begin;
	b_const_itr const next = start == batches.end() - 1 ? batches.begin() : start + 1;

	if (further_repeat == 0) {
		int bdiff = b_end - next;
		if (bdiff < 0)
			bdiff += batches.size();
		if (bdiff >= batches.size() / 2) {
			further_repeat = max_repeat;
			logger.log(reader_waiting_log());
		}
	} else {
		if (b_end != next) {
			further_repeat = max_repeat;
			mergewrite_samples(start, next);
			b_begin = next;
			logger.log(playing_log(next->num));
		} else {
			--further_repeat;
			mergewrite_samples(start, start);
			logger.log(repeat_play_log(start->num));
		}
	}
}

template <class logtype>
void read_log(std::istream &in, logtype &log_m) {
	for_each(log_m.arg_vals, [&in, &log_m](int index, auto &val) {
		in.read(reinterpret_cast<char *>(&val), sizeof(val));
	});
}

template <class logtype>
void partial_log(std::ostream &out, uint32_t timediff, logtype const &log_m) {
	out << timediff << ' ' << log_m.message << "::";
	for_each(log_m.arg_vals, [&out, &log_m](int index, auto const &val) {
		out << ' ' << log_m.arg_names[index] << ": " << val << ";";
	});
	out << std::endl;
}

void bytes_log(std::istream &in, std::ostream &out, char *bytes, int size) {
	in.read(bytes, size);
	for (int i = 0; i < size; ++i) {
		out << uint16_t(uint8_t(bytes[i])) << ' ';
	}
	out << std::endl;
}

template <class logtype>
void text_log(std::istream &in, std::ostream &out, uint32_t timediff, logtype const &log_m) {
	partial_log(out, timediff, log_m);
}

template <>
void text_log(std::istream &in, std::ostream &out, uint32_t timediff, captured_log const &log_m) {
	char bytes[std::get<1>(log_m.arg_vals)];
	partial_log(out, timediff, log_m);
	bytes_log(in, out, bytes, sizeof bytes);
}

template <>
void text_log(std::istream &in, std::ostream &out, uint32_t timediff, validated_log const &log_m) {
	static logger_t logger("recurse.log");
	char bytes[std::get<0>(log_m.arg_vals)];
	partial_log(out, timediff, log_m);
	bytes_log(in, out, bytes, sizeof bytes);
	while (next_play_event < curr_micros) {
		playing_loop(logger);
		next_play_event += inc_micros;
	}
	uint8_t *bts = reinterpret_cast<uint8_t *>(bytes);
	write_packet(bts, get_little_endian(bts + useless_length + uid.size()), logger);
}

#define ZERO_OUT(X) 0
#undef LOG_TYPE
#define LOG_TYPE(ID, NAME, ...)                             \
	case ID: {                                              \
		CONCAT(NAME, _log) val{MAP(ZERO_OUT, __VA_ARGS__)}; \
		read_log(cin, val);                                 \
		text_log(cin, cout, timediff, val);                 \
		break;                                              \
	}

int main() {
	cin.read(reinterpret_cast<char *>(&curr_micros), 8);
	next_play_event = curr_micros + 1;

	uint32_t timediff;
	while ((timediff = cin.get()) != istream::traits_type::eof()) {
		cin.read(reinterpret_cast<char *>(&timediff) + 1, timediff & 1 ? 3 : 1);
		if (~timediff == 0) {
			cout << "Wrong time: ";
		}

		timediff >>= 1;
		curr_micros += timediff;
		cout << curr_micros << ' ';

		switch (cin.get()) {
#include "../src/loglist.h"
		}
	}
	return 0;
}
