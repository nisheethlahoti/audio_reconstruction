#include <sys/select.h>
#include <unistd.h>
#include <iostream>
#include "lib.h"

void trap_error(std::function<void()> func) try { func(); } catch (std::runtime_error &err) {
	std::cerr << err.what() << std::endl;
}

size_t buf_read_available(void *buf, size_t count) {
	return static_cast<size_t>(wrap_error(read(0, buf, count), "reading input"));
}

bool buf_read_blocking(void *buf, size_t count) {
	while (size_t done = buf_read_available(buf, count))
		if ((count -= done) == 0)
			return true;
	return false;
}

size_t buf_write(void const *buf, size_t count) {
	return static_cast<size_t>(wrap_error(write(1, buf, count), "writing output"));
}

static inline size_t check_available(bool block, char const *msg) {
	timeval timeout{};
	fd_set fdset = {};
	FD_SET(0, &fdset);
	return wrap_error(select(1, &fdset, nullptr, nullptr, block ? nullptr : &timeout), msg);
}

void wait_for_input() noexcept(false) { check_available(true, "waiting for input"); }

size_t buf_drain() noexcept(false) {
	char buf[64];
	size_t ret = 0, more;
	do {
		more = wrap_error(read(0, buf, sizeof(buf)), "draining buffer");
		ret += more;
	} while (more == sizeof(buf) && check_available(false, "checking after full drain"));
	return ret;
}
