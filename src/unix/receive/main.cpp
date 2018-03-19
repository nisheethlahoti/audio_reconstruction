#include <signal.h>
#include <soundrex/realtime.h>
#include <unix/soundrex/capture.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "multiplexer.h"

static inline void report_drops(int redundancy, std::vector<capture_t> &captures) {
	static auto time = std::chrono::steady_clock::now();
	auto curr = std::chrono::steady_clock::now();
	auto tdiff = std::chrono::duration_cast<std::chrono::milliseconds>(curr - time).count();
	if (tdiff >= 1000) {
		double expected = samples_per_s * redundancy * tdiff / 1000.0 / packet_samples;
		time = curr;
		std::cerr << "Expected " << expected << " packets in " << tdiff << " ms. Dropped";
		for (capture_t &cap : captures)
			std::cerr << ' ' << expected - cap.getrecv() << " on " << cap.name() << ';';
		std::cerr << '\n';
	}
}

static std::string const filter_str = []() -> auto {
	std::stringstream stream;
	stream << std::hex << std::setw(2) << std::setfill('0') << "ether dst ";
	for (uint16_t byte : pin)
		stream << byte << ':';
	stream.seekp(-1, std::ios::cur);
	stream << " and len - radio[1:2] = " << std::dec << sizeof(packet_t) + 32;
	return stream.str();
}
();

int main(int argc, char **argv) try {
	int const redundancy = std::strtol(argv[1], nullptr, 10);
	if (redundancy <= 0 || errno)
		throw std::invalid_argument(std::string(argv[0]) + " <redundancy> <ifaces...>");

	multiplexer_t multiplexer;
	multiplexer.add_fd(0);  // stdin

	std::cerr << "Filter set to (" << filter_str << ")\n";
	std::vector<capture_t> captures = open_captures(argc - 2, argv + 2);
	for (capture_t &cap : captures) {
		cap.setfilter(filter_str.c_str());
		multiplexer.add_fd(cap.fd());
	}

	set_realtime();

	std::cerr << std::fixed << std::setprecision(2);
	std::cerr << "Press Enter to toggle corrections and Ctrl+d to quit\n";

	while (std::cin) {
		multiplexer.next();
		if (multiplexer.is_ready(0)) {
			std::string str;
			std::getline(std::cin, str);  // Dunno why cin.ignore isn't working
			kill(0, SIGURG); // TODO: Find less horrible way that this.
		}

		report_drops(redundancy, captures);
		for (capture_t &cap : captures)
			if (multiplexer.is_ready(cap.fd())) {
				auto const packet = cap.get_packet();
				std::cout.write(reinterpret_cast<char const *>(packet.data), packet.size);
			}
	}
} catch (std::exception const &expt) {
	std::cerr << "Terminated abnormally: " << expt.what() << '\n';
	return 1;
}
