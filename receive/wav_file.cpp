#include <fstream>
#include "wav_file.h"

using namespace std;

template<typename type>
void binary_write(ostream &out, type val) {
	out.write(reinterpret_cast<char*>(&val), sizeof val);
}

void wav_t::write_to_file(char const *filename) {
	uint32_t data_size = header.block_align * samples.size();
	ofstream outfile(filename, ios::out | ios::binary);

	outfile << "RIFF";
	binary_write<uint32_t>(outfile, 4 /*remaining data in this chunk*/ + (8 + sizeof header)/*fmt chunk*/ + (8+data_size)/*data chunk*/);
	outfile << "WAVE";

	outfile << "fmt ";
	binary_write<uint32_t>(outfile, sizeof header);
	binary_write(outfile, header);

	outfile << "data";
	binary_write<uint32_t>(outfile, data_size);
	for (sample_t const &sample: samples)
		binary_write(outfile, sample);

	outfile.close();
}

