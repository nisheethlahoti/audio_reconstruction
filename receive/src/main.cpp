#include <sys/select.h>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "capture.h"
#include "receive.h"

// Just a C++ style wrapper around the select() syscall
class multiplexer_t {
	fd_set fdset = {}, tmpset;
	int fdmax = -1;

   public:
	void add_fd(int fd) {
		FD_SET(fd, &fdset);
		fdmax = std::max(fd, fdmax);
	}

	inline void next() {
		tmpset = fdset;
		if (select(fdmax + 1, &tmpset, nullptr, nullptr, nullptr) == -1) {
			perror("select call");
			exit(1);
		}
	}

	inline bool is_ready(int fd) const { return FD_ISSET(fd, &tmpset); }
};

static inline void prompt_input(bool corr) {
	std::cerr << (corr ? "Corrections on.\n" : "Corrections off.\n");
}

int main(int argc, char **argv) {
	std::vector<capture_t> captures;
	multiplexer_t multiplexer;
	multiplexer.add_fd(0);  // stdin

	for (int i = 1; i < argc; ++i) {
		captures.emplace_back(argv[i]);
		if (captures.back().fd() == -1)
			captures.pop_back();
		else
			multiplexer.add_fd(captures.back().fd());
	}

	std::cerr << captures.size() << " interfaces opened for capture." << std::endl;
	if (captures.empty())
		return 1;

	logger_t playlogger("play.bin"), packetlogger("packet.bin");
	std::thread(playing_loop, std::ref(playlogger)).detach();
	initialize_player();
	std::cerr << std::fixed << std::setprecision(2);

	std::cerr << "Enter (c) to toggle corrections and (q) to quit.\n";
	prompt_input(correction_on);

	auto time = std::chrono::steady_clock::now();
	while (true) {
		multiplexer.next();
		if (multiplexer.is_ready(0)) {
			std::string str;
			std::getline(std::cin, str);
			if (str[0] == 'c') {
				bool corr = correction_on.load(std::memory_order_relaxed);
				correction_on.store(!corr, std::memory_order_release);
				prompt_input(!corr);
			} else if (str[0] == 'q') {
				return 0;
			}
		}

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

		for (capture_t &cap : captures)
			if (multiplexer.is_ready(cap.fd()))
				if (receive_callback(cap.get_packet(), packetlogger))
					cap.addrecv();
	}
}
