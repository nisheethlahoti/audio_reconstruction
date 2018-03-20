#include <soundrex/constants.h>
#include <unix/soundrex/main.h>
#include <array>
#include <cstring>
#include <iostream>
#include <random>

void soundrex_main(slice_t<char *> args) {
	char *postp, *postnum;
	if (args.size() < 2)
		throw std::runtime_error("Too few arguments: <drop_chance> [<starting_num>]");

	packet_t buf{};
	std::default_random_engine gen(12345);
	std::bernoulli_distribution dist(std::strtod(args[1], &postp));

	if (args.size() > 2)
		buf.num = std::strtoul(args[2], &postnum, 0);

	if (*postp || dist.p() < 0 || dist.p() > 1 || (args.size() > 2 && *postnum) || errno)
		throw std::runtime_error("Invalid argument: <drop_chance> [<starting_num>]");

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
}
