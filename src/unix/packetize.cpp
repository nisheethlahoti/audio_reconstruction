#include <soundrex/constants.h>
#include <unix/soundrex/common.h>
#include <array>
#include <cstring>
#include <iostream>
#include <random>

int main(int argc, char **argv) try {
	if (argc < 2)
		throw std::runtime_error("Too few arguments");

	packet_t buf{};
	std::default_random_engine gen(12345);
	std::bernoulli_distribution dist(std::strtod(argv[1], &argv[1]));

	set_realtime();
	if (argc > 2)
		buf.num = std::strtoul(argv[2], &argv[2], 0);

	if (argv[1][0] || dist.p() < 0 || dist.p() > 1 || (argc > 2 && argv[2][0]) || errno)
		throw std::runtime_error("Invalid argument");

	std::cerr << "Dropping packets with probability " << dist.p() << '\n';
	unsigned incr = std::max(1u, unsigned(std::chrono::milliseconds(500) / duration));

	std::string tname;
	std::getline(std::cin, tname);
	std::cerr << "Setting terminal to " << tname << '\n';

	while (std::cin.read(reinterpret_cast<char *>(buf.samples.data() + buf.trailing.size()),
	                     sizeof(buf.samples))) {
		if (++buf.num % incr == 0)
			std::cerr << "\rSending packet " << buf.num;

		if (!dist(gen))
			std::cout.write(reinterpret_cast<char *>(&buf), sizeof(buf));  // assuming little endian
		std::memcpy(buf.samples.data(), buf.trailing.data(), sizeof(buf.trailing));
	}

	std::cerr << std::endl;
} catch (std::runtime_error &err) {
	std::cerr << err.what() << "; Usage: " << argv[0] << " <drop_chance> [<starting_num>]\n";
	return 1;
}
