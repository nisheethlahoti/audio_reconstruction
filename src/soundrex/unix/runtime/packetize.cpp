#include <soundrex/constants.h>
#include <soundrex/unix/runtime/lib.h>
#include <array>
#include <cstring>
#include <iostream>
#include <random>

void soundrex_main(slice_t<char *> args) {
	char *postp, *postnum;
	if (args.empty())
		throw std::domain_error("<drop_chance> [<starting_num>]");

	double const prob = std::strtod(args[0], &postp);
	long const num = args.size() > 1 ? std::strtol(args[1], &postnum, 0) : 0;
	if (*postp || prob < 0 || prob > 1 || (args.size() > 1 && *postnum) || num < 0)
		throw std::domain_error("<drop_chance (0.0-1.0)> [<starting_num (unsigned 32-bit)>]");

	packet_t buf{static_cast<uint32_t>(num), {}, {}};
	std::default_random_engine gen(12345);
	std::bernoulli_distribution dist(prob);

	std::cerr << "Dropping packets with probability " << dist.p() << '\n';
	unsigned incr = std::max(1u, unsigned(std::chrono::milliseconds(500) / duration));

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
