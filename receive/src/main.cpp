#include <pthread.h>
#include <sched.h>
#include <sys/select.h>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "../../realtime.h"
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

static void report_drops(std::vector<capture_t> &captures) {
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

static void playing_loop() {
	logger_t playlogger("play.bin");
	set_realtime(1, 1);
	auto time = std::chrono::steady_clock::now();
	while (std::cin) {
		std::this_thread::sleep_until(time += duration);
		play_next(playlogger);
	}
}

void write_samples(void const *samples, size_t len) {
	std::cout.write(static_cast<char const *>(samples), len * sizeof sample_t());
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

	logger_t packetlogger("packet.bin");
	std::thread player(playing_loop);
	set_realtime(1, 2);

	std::ios::sync_with_stdio(false);
	std::cerr << std::fixed << std::setprecision(2);
	std::cerr << "Enter (c) to toggle corrections and (q) to quit.\n";

	while (std::cin) {
		multiplexer.next();
		if (multiplexer.is_ready(0)) {
			bool corr = correction_on.load(std::memory_order_relaxed);
			correction_on.store(!corr, std::memory_order_release);
			std::cerr << (corr ? "Corrections off.\n" : "Corrections on.\n");
			std::string str;
			std::getline(std::cin, str);  // Dunno why cin.ignore isn't working
		}

		report_drops(captures);
		for (capture_t &cap : captures)
			if (multiplexer.is_ready(cap.fd()))
				if (receive_callback(cap.get_packet(), packetlogger))
					cap.addrecv();
	}

	player.join();
	return 0;
}
