#ifndef SOUNDREX_LOGGER_H
#define SOUNDREX_LOGGER_H

#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <tuple>
#include <utility>

#include "../../generic_macros.h"

template <class tuple_t, size_t... sizes, class fn_t>
static inline void for_each_impl(std::index_sequence<sizes...>, tuple_t &&tup, fn_t &&fn) {
	(fn(sizes, std::get<sizes>(tup)), ...);
}

template <class tuple_t, class fn_t>
inline void for_each(tuple_t &&tup, fn_t &&fn) {
	for_each_impl(std::make_index_sequence<std::tuple_size<decltype(std::tuple_cat(tup))>::value>{},
	              tup, fn);
}

#define APPLYCAR(X) CAR X
#define APPLYCDR(X) CDR X
#define STRINGCDR(X) STRING(CDR X)

#define WRITE(X, Y) X Y
#define APPLYWRITE(X) WRITE X

// Because C++ doesn't have reflection :/
#define LOG_TYPE(ID, NAME, ...)                                                    \
	struct CONCAT(NAME, _log) {                                                    \
		static constexpr char message[] = #NAME;                                   \
		static constexpr uint8_t id = ID;                                          \
		static constexpr std::array<char const *, NUMARGS(__VA_ARGS__)> arg_names{ \
		    {MAP(STRINGCDR, __VA_ARGS__)}};                                        \
		std::tuple<MAP(APPLYCAR, __VA_ARGS__)> arg_vals;                           \
		CONCAT(NAME, _log)                                                         \
		(MAP(APPLYWRITE, __VA_ARGS__)) : arg_vals({MAP(APPLYCDR, __VA_ARGS__)}) {} \
	};

#include "loglist.h"

/// Thread-unsafe logging class.
class logger_t {
	std::chrono::time_point<std::chrono::steady_clock> time;
	std::ofstream logfile;
	uint32_t get_timediff();

   public:
	logger_t(char const *fname);
	~logger_t();

	void write(void const *bytes, size_t size);
	template <class logtype>
	inline void log(logtype const &log_m) {
		uint32_t timediff = get_timediff() << 1;
		if (__builtin_expect(timediff >> 16, 0)) {
			timediff = timediff | 1;
			logfile.write(reinterpret_cast<char const *>(&timediff), 4);
		} else {
			logfile.write(reinterpret_cast<char const *>(&timediff), 2);
		}
		logfile.put(log_m.id);

		for_each(log_m.arg_vals, [this, &log_m](int index, auto const &val) {
			logfile.write(reinterpret_cast<char const *>(&val), sizeof(val));
		});
	}
};

#endif
