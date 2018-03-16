#include <algorithm>
#include <array>
#include <iostream>

#include <constants.h>
#include <realtime.h>

uint32_t packet_num = 1;

void fill_packet(uint8_t *pos) {
	for (int i = 0; i < 4; ++i) {
		pos[i] = (packet_num >> (8 * i)) & 0xff;
		pos[i + 4] = uid[i] ^ magic_number[i] ^ pos[i];
	}
	pos += 8;

	constexpr size_t psize = packet_samples * byte_depth * num_channels,
	                 tsize = trailing_samples * byte_depth * num_channels;

	std::copy(pos + psize, pos + psize + tsize, pos);
	std::cin.read(reinterpret_cast<char *>(pos + tsize), psize);
	++packet_num;
}

int main(int argc, char **argv) {
	if (argc > 1)
		packet_num = atoi(argv[1]);

	std::array<uint8_t, packet_size> buf;

	std::copy(uid.begin(), uid.end(), buf.begin());
	uint8_t *const packet_loc = buf.data() + uid.size();

	set_realtime();
	for (int itr = 0; std::cin; ++itr) {
		if (itr % 1000 == 0)
			std::cerr << "Sending next thousand packets\n";

		fill_packet(packet_loc);
		std::cout.write(reinterpret_cast<char *>(buf.data()), buf.size());
	}

	return 0;
}
