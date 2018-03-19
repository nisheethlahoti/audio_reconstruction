#include <string>
#include <system_error>
#include "multiplexer.h"

void multiplexer_t::add_fd(int fd) {
	FD_SET(fd, &fdset);
	fdmax = fd > fdmax ? fd : fdmax;
}

void multiplexer_t::next() {
	tmpset = fdset;
	if (select(fdmax + 1, &tmpset, nullptr, nullptr, nullptr) == -1)
		throw std::system_error(errno, std::system_category(), "select call");
}

bool multiplexer_t::is_ready(int fd) const { return FD_ISSET(fd, &tmpset); }
