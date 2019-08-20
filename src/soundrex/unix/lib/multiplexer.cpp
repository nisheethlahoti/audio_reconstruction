#include <string>

#include <soundrex/unix/lib.hpp>
#include "multiplexer.hpp"

void multiplexer_t::add_fd(int fd) {
	FD_SET(fd, &fdset);
	fdmax = fd > fdmax ? fd : fdmax;
}

void multiplexer_t::next() {
	tmpset = fdset;
	wrap_error(select(fdmax + 1, &tmpset, nullptr, nullptr, nullptr), "select call");
}

bool multiplexer_t::is_ready(int fd) const { return FD_ISSET(fd, &tmpset); }
