#include <signal.h>
#include <soundrex/unix/lib/multiplexer.h>
#include <soundrex/unix/runtime/lib.h>
#include <unistd.h>
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

	std::clog << "Will drop with probability " << dist.p() << " when required" << std::endl;
	unsigned incr = std::max(1u, unsigned(std::chrono::milliseconds(500) / duration));
	bool extra_drops = true;

	multiplexer_t multiplexer;
	multiplexer.add_fd(0);  // For incoming sample stream
	multiplexer.add_fd(2);  // For terminal user input

	uint8_t *const bstart = reinterpret_cast<uint8_t *>(buf.samples.data() + buf.trailing.size());
	uint8_t *const bend = reinterpret_cast<uint8_t *>(buf.trailing.end());
	uint8_t *pos = bstart;

	while (multiplexer.next(), true) {
		if (multiplexer.is_ready(0)) {
			if (size_t done = buf_read_available(pos, bend - pos); !done) {
				std::clog << "End of input stream" << std::endl;
				break;
			} else if ((pos += done) == bend) {
				if (++buf.num % incr == 0)
					std::clog << buf.num << " sent\r" << std::flush;

				if (!extra_drops || !dist(gen))
					buf_write(&buf, sizeof(buf));  // assuming little endian

				std::memcpy(buf.samples.data(), buf.trailing.data(), sizeof(buf.trailing));
				pos = bstart;
			}
		} else if (size_t val = buf_drain(2); val == 0)
			break;
		else if (val == 1) {
			extra_drops = !extra_drops;
			std::clog << "Extra drops " << (extra_drops ? "on" : "off") << std::endl;
		} else
			kill(0, SIGURG);  // TODO: Find less horrible way that this.
	}
}
