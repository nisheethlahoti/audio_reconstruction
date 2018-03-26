#include <soundrex/logtypes.h>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <tuple>
#include <utility>

/// Thread-unsafe logging class.
class logger_t {
	std::chrono::time_point<std::chrono::steady_clock> time;
	std::ofstream logfile;
	uint32_t get_timediff();

   public:
	logger_t(char const *fname);

	template <class logtype>
	inline void log(logtype log_m) {
		uint32_t timediff = get_timediff() << 1;
		if (__builtin_expect(timediff >> 16, 0)) {
			timediff = timediff | 1;
			logfile.write(reinterpret_cast<char const *>(&timediff), 4);
		} else {
			logfile.write(reinterpret_cast<char const *>(&timediff), 2);
		}
		logfile.put(log_m.id);

		for_each(log_m.arg_vals, [this](int, auto const &val) {
			logfile.write(reinterpret_cast<char const *>(&val), sizeof(val));
		});
	}
};
