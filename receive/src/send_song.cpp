// WIFI CRC POLYNOMIAL IS 1 00000100 11000001 00011101 10110111

#include <signal.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

#include "receive.h"

using namespace std;
ofstream logfile("log.bin");
mutex logmut;

uint32_t packet_num = 1;

// TODO: Infer from reading the input file, directly.
constexpr size_t input_depth = 2;
constexpr size_t input_channels = 2;

array<uint8_t, 32> const mac_header = {{
    /*0*/ 0x08, 0x00,  // Frame Control for data frame
    /*2*/ 0x01, 0x01,  // Duration
    /*4*/ 0x01, 0x00, 0x5e, 0x1c, 0x04, 0x5e,
    /*10*/ 0x6e, 0x40, 0x08, 0x49, 0x01, 0x64,  // Source address
    /*16*/ 0x6e, 0x40, 0x08, 0x49, 0x01, 0x64,  // BSSID
    /*22*/ 0x00, 0x00,                          // Seq-ctl

    // addr4 is not present if not WDS(bridge)
    // IPLLC SNAP header : next 2 bytes, SNAP field : next 6 bytes
    /*24*/ 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00  // end
}};

struct wav_chunk {
	std::string name;
	uint32_t size = 0;
	uint8_t *data = nullptr;
	void destroy() { delete[] data; }
};

class wav_reader final : public std::ifstream {
   public:
	wav_reader(char const *fname) : std::ifstream(fname) {}

	wav_chunk readChunk() {
		char temp[5] = {};
		wav_chunk ret;

		read(temp, 4);
		ret.name = std::string(temp);

		read(temp, 4);
		if (ret.name == "RIFF")
			ret.size = 4;
		else
			for (int j = 0; j < 4; ++j)
				ret.size |= (uint8_t)temp[j] << (uint32_t)(8 * j);

		ret.data = new uint8_t[ret.size];
		read((char *)ret.data, ret.size);
		return ret;
	}
};

struct wav_song {
	wav_chunk chunk;

	wav_song(char const *fname) {
		wav_reader reader(fname);
		while ((chunk = reader.readChunk()).name != "data") {
			std::cerr << chunk.name << std::endl;
			chunk.destroy();
		}
		reader.close();
	}

	int32_t value(int const pos, int const channel) const {
		int32_t ret = 0;
		uint8_t const *const ptr =
		    chunk.data + input_depth * (input_channels * pos + channel);
		for (int i = 0; i < input_depth; ++i)
			ret |= static_cast<uint32_t>(ptr[i]) << (8 * i);
		ret = (ret << (4-input_depth)) >> (4-input_depth);
		return ret;
	}
} song("filename.wav");

template <typename iterator>
void copy_and_shift(iterator begin, iterator end, uint8_t *&loc) {
	copy(begin, end, loc);
	loc += end - begin;
}

int32_t shift(int32_t ret, int amount) {
	return amount > 0 ? (ret << amount) : (ret >> (-amount));
}

size_t song_pos = 0;
uint8_t *fill_packet(uint8_t *pos) {
	for (int i = 0; i < 4; ++i) {
		pos[i] = (packet_num >> (8 * i)) & 0xff;
		pos[i + 4] = pos[i] ^ magic_number[i];
	}
	pos += 8;

	for (int i = 0; i < total_samples; ++i) {
		for (int t = 0; t < num_channels; ++t) {
			int32_t val = shift(
			    song.value(song_pos, t),
			    8 * (byte_depth - input_depth));  // Converting 16 bit to required size.
			for (int j = 0; j < byte_depth; ++j) {
				pos[byte_depth * num_channels * i + t * byte_depth + j] =
				    (val >> (8 * j)) & 0xff;
			}
		}
		song_pos++;
	}
	++packet_num;

	song_pos -= trailing_samples;
	return byte_depth * num_channels * total_samples + pos;
}

int main(int argc, char **argv) {
	signal(SIGINT, exit);
	if (argc < 2) {
		cerr << "Not enough arguments\n";
		return 1;
	}

	default_random_engine gen(12345);
	bernoulli_distribution dist(atof(argv[1]));

	uint8_t buf[2000];
	uint8_t *packet_loc = buf;

	copy_and_shift(mac_header.begin(), mac_header.end(), packet_loc);
	copy_and_shift(uid.begin(), uid.end(), packet_loc);

	initialize_player();
	auto time = chrono::steady_clock::now();
	thread(playing_loop, time).detach();

	// Because input file is 16 bit stereo
	for (int itr = 0; song_pos + total_samples < song.chunk.size / 4; ++itr) {
		if (itr % 1000 == 0)
			cout << "Sending next thousand packets" << endl;

		uint8_t *endptr = fill_packet(packet_loc);
		this_thread::sleep_until(time += duration);
		if (!dist(gen))
			receive_callback(buf, endptr - buf);
	}
	return 0;
}
