#include <sys/select.h>
#include <unistd.h>
#include "lib.h"

void trap_error(std::function<void()> func) try { func(); } catch (std::runtime_error &err) {
	std::fprintf(stderr, "%s\n", err.what());
}

size_t buf_read_available(void *buf, size_t count) {
	return static_cast<size_t>(wrap_error(read(0, buf, count), "reading input"));
}

bool buf_read_blocking(void *const buf, size_t count) {
	char *buffer = static_cast<char *>(buf);
	while (size_t done = buf_read_available(buffer, count)) {
		buffer += done;
		if ((count -= done) == 0)
			return true;
	}
	return false;
}

size_t buf_write(void const *buf, size_t count) {
	return static_cast<size_t>(wrap_error(write(1, buf, count), "writing output"));
}

static inline size_t check_available(int fd, bool block, char const *msg) {
	timeval timeout{};
	fd_set fdset = {};
	FD_SET(fd, &fdset);
	return wrap_error(select(fd + 1, &fdset, nullptr, nullptr, block ? nullptr : &timeout), msg);
}

void wait_for_input() noexcept(false) { check_available(0, true, "waiting for input"); }

size_t buf_drain(int fd) noexcept(false) {
	char buf[64];
	size_t ret = 0, more;
	do {
		more = wrap_error(read(fd, buf, sizeof(buf)), "draining buffer");
		ret += more;
	} while (more == sizeof(buf) && check_available(fd, false, "checking after full drain"));
	return ret;
}
