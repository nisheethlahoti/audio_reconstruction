#include <fcntl.h>
#include <cstring>

#include "logger.hpp"

logger_t::logger_t(char const *fname)
    : fd(wrap_error(open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0664), fname)) {
	using namespace std::chrono;
	time = steady_clock::now();
	uint64_t us = duration_cast<microseconds>(time.time_since_epoch()).count();
	fd_write(&us, 8);
}

uint32_t logger_t::get_timediff() {
	using namespace std::chrono;
	auto const prev = time;
	time = steady_clock::now();
	auto const val = duration_cast<microseconds>(time - prev).count();
	return val >> 31 ? ~uint32_t(0) : val;
}

void logger_t::fd_write(void const *buf, size_t count) {
	std::memcpy(buffer.data() + bufpos, buf, count);
	bufpos += count;
}

logger_t::~logger_t() {
	wrap_error(write(fd, buffer.data(), bufpos), "Logging to file");
	wrap_error(close(fd), "Closing log file");
}
