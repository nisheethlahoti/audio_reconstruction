#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include <realtime.h>
#include <receiver/receiver.h>
#include <receiver/unix/capture.h>
#include <receiver/unix/multiplexer.h>

static std::vector<capture_t> captures;
static multiplexer_t multiplexer;
static receiver_t receiver;

static inline void open_captures(int num, char **names) {
	captures.reserve(num);
	for (int i = 0; i < num; ++i) {
		captures.emplace_back(names[i]);
		if (captures.back().fd() == -1)
			captures.pop_back();
	}

	std::cerr << captures.size() << " interfaces opened for capture." << std::endl;
}

static inline void report_drops() {
	static auto time = std::chrono::steady_clock::now();
	auto curr = std::chrono::steady_clock::now();
	auto tdiff = std::chrono::duration_cast<std::chrono::milliseconds>(curr - time).count();
	if (tdiff >= 1000) {
		double expected = samples_per_s * 3 * tdiff / 1000.0 / packet_samples;
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
	open_captures(argc - 1, argv + 1);
	multiplexer.add_fd(0);  // stdin
	for (capture_t const &cap : captures)
		multiplexer.add_fd(cap.fd());

	set_realtime();
	std::thread player(playing_loop);

	std::cerr << std::fixed << std::setprecision(2);
	std::cerr << "Enter (c) to toggle corrections and (q) to quit.\n";

	while (std::cin) {
		multiplexer.next();
		if (multiplexer.is_ready(0)) {
			std::string str;
			std::getline(std::cin, str);  // Dunno why cin.ignore isn't working
			std::cerr << (receiver.toggle_corrections() ? "Correcting\n" : "Not correcting\n");
		}

		report_drops();
		for (capture_t &cap : captures)
			if (multiplexer.is_ready(cap.fd()))
				if (receiver.receive_callback(cap.get_packet()))
					cap.addrecv();
	}

	player.join();
	return 0;
}
