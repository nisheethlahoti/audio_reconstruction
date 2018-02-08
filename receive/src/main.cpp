#include <sys/select.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "receive.h"
#include "capture.h"

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
	std::cerr << "Press enter to stop..." << std::endl;
	while (multiplexer.next(), !multiplexer.is_ready(0)) {
		for (capture_t const &cap : captures)
			if (multiplexer.is_ready(cap.fd()))
				receive_callback(cap.get_packet(), packetlogger);
	}

	return 0;
}
