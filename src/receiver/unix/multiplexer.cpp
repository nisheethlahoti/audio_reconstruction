#include <cstdio>
#include <cstdlib>

#include <receiver/unix/multiplexer.h>

void multiplexer_t::add_fd(int fd) {
	FD_SET(fd, &fdset);
	fdmax = fd > fdmax ? fd : fdmax;
}

void multiplexer_t::next() {
	tmpset = fdset;
	if (select(fdmax + 1, &tmpset, nullptr, nullptr, nullptr) == -1) {
		perror("select call");
		exit(1);
	}
}

bool multiplexer_t::is_ready(int fd) const { return FD_ISSET(fd, &tmpset); }
