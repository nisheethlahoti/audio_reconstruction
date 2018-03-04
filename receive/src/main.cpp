#include <sys/select.h>
#include <cstdio>
#include <cstdlib>
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

static inline void prompt_input() {
	std::cerr << (correction_on ? "Corrections on." : "Corrections off.");
	std::cerr << " Enter (c) to toggle corrections and (q) to quit: ";
	std::cerr.flush();
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

	prompt_input();
	while (true) {
		multiplexer.next();
		if (multiplexer.is_ready(0)) {
			std::string str;
			std::getline(std::cin, str);
			if (str[0] == 'c') {
				correction_on = !correction_on;
				prompt_input();
			} else if (str[0] == 'q') {
				return 0;
			}
		}

		for (capture_t const &cap : captures)
			if (multiplexer.is_ready(cap.fd()))
				receive_callback(cap.get_packet(), packetlogger);
	}
}
