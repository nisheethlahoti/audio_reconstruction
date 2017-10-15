#include <cstdint>
#include <vector>
#include "receive.h"

typedef int16_t wav_sample_t;

struct wav_header_t {
	uint16_t audio_format = 1;
	uint16_t num_ch;
	uint32_t sample_rate, byte_rate;
	uint16_t block_align, bits_per_sample;

	wav_header_t():
		num_ch(num_channels),
		sample_rate(samples_per_s),
		byte_rate(samples_per_s * num_channels * byte_depth),
		block_align(num_channels * byte_depth),
		bits_per_sample(byte_depth * 8)
	{}
};

struct wav_t {
	wav_header_t const header;
	std::vector<wav_sample_t> samples;
	void write_to_file(char const *filename);
};
