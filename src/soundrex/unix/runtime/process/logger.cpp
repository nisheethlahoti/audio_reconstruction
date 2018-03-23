#include "logger.h"

logger_t::logger_t(char const *fname) : logfile(fname, std::ios::binary) {
	using namespace std::chrono;
	time = steady_clock::now();
	uint64_t us = duration_cast<microseconds>(time.time_since_epoch()).count();
	logfile.write(reinterpret_cast<char const *>(&us), 8);
}

uint32_t logger_t::get_timediff() {
	using namespace std::chrono;
	auto const prev = time;
	time = steady_clock::now();
	auto const val = duration_cast<microseconds>(time - prev).count();
	return val >> 31 ? ~uint32_t(0) : val;
}
