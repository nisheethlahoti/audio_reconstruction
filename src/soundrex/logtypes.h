#ifndef SOUNDREX_LOGTYPES_H
#define SOUNDREX_LOGTYPES_H

#include <soundrex/generic_macros.h>
#include <cstdint>
#include <tuple>
#include <utility>

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

#include <soundrex/logtypes.list>
#undef LOG_TYPE

#endif
