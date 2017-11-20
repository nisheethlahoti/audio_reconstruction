#ifndef SOUNDREX_WAV_TOOLS_UTIL_H
#define SOUNDREX_WAV_TOOLS_UTIL_H

#include <cstdint>
#include <fstream>
#include <utility>

template <typename type>
void binary_write(std::ostream &out, type val, size_t size = sizeof type()) {
	out.write(reinterpret_cast<char *>(&val), size);
}

static std::pair<std::string, uint32_t> readChunkInfo(std::istream &file) {
	char temp[4] = {};
	file.read(temp, 4);
	std::pair<std::string, uint32_t> ret = std::make_pair(std::string(temp), 0);
	file.read(temp, 4);

	if (ret.first == "RIFF")
		ret.second = 4;
	else
		for (int j = 0; j < 4; ++j)
			ret.second |= (uint8_t)temp[j] << (uint32_t)(8 * j);

	return ret;
}

struct wav_header_t {
	uint16_t audio_format = 1;
	uint16_t num_ch;
	uint32_t sample_rate, byte_rate;
	uint16_t block_align, bits_per_sample;

	wav_header_t(uint16_t num_channels = 0, uint32_t samples_per_s = 0,
	             uint16_t byte_depth = 0)
	    : num_ch(num_channels),
	      sample_rate(samples_per_s),
	      byte_rate(samples_per_s * num_channels * byte_depth),
	      block_align(num_channels * byte_depth),
	      bits_per_sample(byte_depth << 3) {}

	inline uint32_t sample_bytes() const {
		return num_ch * (bits_per_sample >> 3);
	}
};

#endif
