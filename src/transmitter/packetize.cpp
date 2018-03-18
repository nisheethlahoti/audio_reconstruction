#include <array>
#include <cstring>
#include <iostream>

#include <constants.h>
#include <realtime.h>

int main(int argc, char **argv) {
	packet_t buf{};
	if (argc > 1)
		buf.num = atoi(argv[1]);

	set_realtime();
	while (std::cin.read(reinterpret_cast<char *>(buf.samples.data() + buf.trailing.size()),
	                     sizeof(buf.samples))) {
		if (buf.num++ % 1000 == 0)
			std::cerr << "Sending next thousand packets\n";

		std::cout.write(reinterpret_cast<char *>(&buf), sizeof(buf));  // assuming little endian
		std::memcpy(buf.samples.data(), buf.trailing.data(), sizeof(buf.trailing));
	}

	return 0;
}
