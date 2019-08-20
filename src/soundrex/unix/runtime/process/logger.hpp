#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <tuple>
#include <utility>

#include <soundrex/logtypes.hpp>
#include <soundrex/unix/lib.hpp>

/// Thread-unsafe logging class.
class logger_t {
	std::chrono::time_point<std::chrono::steady_clock> time;
	int const fd;
	uint16_t bufpos = 0;
	std::array<uint8_t, 4096> buffer;
	uint32_t get_timediff();
	void fd_write(void const *buf, size_t count) noexcept(false);

   public:
	logger_t(char const *fname);
	logger_t(logger_t &) = delete;
	logger_t(logger_t &&) = delete;
	~logger_t();

	template <class logtype>
	inline void log(logtype log_m) noexcept(false) {
		if (bufpos > buffer.size() - (5 + logtype::argsize)) {
			wrap_error(write(fd, buffer.data(), bufpos), "Logging to file");
			bufpos = 0;
		}

		uint32_t timediff = get_timediff() << 1;
		if (__builtin_expect(timediff >> 16, 0)) {
			timediff = timediff | 1;
			fd_write(&timediff, 4);
		} else {
			fd_write(&timediff, 2);
		}
		buffer[bufpos++] = log_m.id;

		for_each(log_m.arg_vals, [this](int, auto const &val) { fd_write(&val, sizeof(val)); });
	}
};
