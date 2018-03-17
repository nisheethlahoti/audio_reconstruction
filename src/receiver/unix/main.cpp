#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include <realtime.h>
#include <receiver/receiver.h>
#include <receiver/unix/multiplexer.h>
#include <unix/capture.h>

static receiver_t receiver;

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

static inline void playing_loop() {
	auto time = std::chrono::steady_clock::now();
	while (std::cin) {
		std::this_thread::sleep_until(time += duration);
		receiver.play_next();
	}
}

int main(int argc, char **argv) {
	int const redundancy = std::strtol(argv[1], nullptr, 10);
	if (redundancy <= 0 || errno) {
		std::cerr << "Usage: " << argv[0] << " <redundancy> <interfaces...>\n";
		return 1;
	}

	multiplexer_t multiplexer;
	std::vector<capture_t> captures = open_captures(argc - 2, argv + 2);

	multiplexer.add_fd(0);  // stdin
	for (capture_t const &cap : captures)
		multiplexer.add_fd(cap.fd());

	set_realtime();
	std::thread player(playing_loop);

	std::cerr << std::fixed << std::setprecision(2);
	std::cerr << "Press Enter to toggle corrections and Ctrl+d to quit\n";

	while (std::cin) {
		multiplexer.next();
		if (multiplexer.is_ready(0)) {
			std::string str;
			std::getline(std::cin, str);  // Dunno why cin.ignore isn't working
			std::cerr << (receiver.toggle_corrections() ? "Correcting\n" : "Not correcting\n");
		}

		report_drops(redundancy, captures);
		for (capture_t &cap : captures)
			if (multiplexer.is_ready(cap.fd()))
				if (receiver.receive_callback(cap.get_packet()))
					cap.addrecv();
	}

	player.join();
	return 0;
}
