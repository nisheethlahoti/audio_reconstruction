/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Class for writing to WAVE file as soon as a sample is received. Completes *
 * proper write on destruction. (File can be in a bad state if not destroyed *
 * properly.                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <cassert>

#include "util.h"

template <typename sample_t>
class wav_writer_t {
	wav_header_t const header;
	std::ofstream outp;
	uint32_t count = 0;

   public:
	void write_to_file(char const *filename) {}

	wav_writer_t(char const *filename, wav_header_t const &hdr)
	    : outp(filename), header(hdr) {
		assert(sizeof sample_t() == header.block_align);
		assert(header.block_align >=
		       header.bits_per_sample / 8 * header.num_ch);

		outp << "RIFF";
		binary_write<uint32_t>(outp, 0);
		outp << "WAVE";

		outp << "fmt ";
		binary_write<uint32_t>(outp, sizeof header);
		binary_write(outp, header);

		outp << "data";
		binary_write<uint32_t>(outp, 0);
	}

	void add_sample(sample_t const &sample) {
		binary_write(outp, sample);
		count++;
	}

	~wav_writer_t() {
		uint32_t data_size = count * sizeof sample_t();
		outp.seekp(4, std::ios::beg);
		binary_write<uint32_t>(outp,
		                       4 /*remaining data in this chunk*/
		                           + (8 + sizeof header) /*fmt chunk*/
		                           + (8 + data_size) /*data chunk*/);
		outp.seekp(12 + 8 + sizeof header + 4, std::ios::beg);
		binary_write(outp, data_size);
		outp.close();
	}
};
