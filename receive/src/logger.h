#ifndef SOUNDREX_LOGGER_H
#define SOUNDREX_LOGGER_H

#include <array>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <utility>

#include "../../generic_macros.h"

template <class tuple_t, size_t... sizes, class func_t>
void for_each_impl(tuple_t&& tup, std::index_sequence<sizes...> sz, func_t&& func) {
	(func(sizes, std::get<sizes>(tup)),...);
}

template <class tuple_t, class func_t>
void for_each(tuple_t&& tup, func_t&& func) {
	for_each_impl(tup, std::make_index_sequence<std::tuple_size<decltype(std::tuple_cat(tup))>::value>{}, func);
}

template <class logtype>
void text_log(std::ostream &out, logtype const &log_m) {
	out << log_m.message << ":: ";
	for_each(log_m.arg_vals, [&out, &log_m] (int index, auto const &val) {
		out << log_m.arg_names[index] << ": " << val << "; ";
	});
	out << std::endl;
}

template <class logtype>
void binary_log(std::ostream &out, logtype const &log_m) {
	out.put(log_m.id);
	for_each(log_m.arg_vals, [&out, &log_m] (int index, auto const &val) {
		out.write(reinterpret_cast<char const*>(&val), sizeof(val));
	});
}

#define APPLYCAR(X) CAR X
#define APPLYCDR(X) CDR X
#define STRINGCDR(X) STRING(CDR X)

#define WRITE(X,Y) X Y
#define APPLYWRITE(X) WRITE X

// Because C++ doesn't have reflection :/
#define LOG_TYPE(ID,NAME,...) struct CONCAT(NAME,_log) {\
	static constexpr char message[] = #NAME;\
	static constexpr uint8_t id = ID;\
	static constexpr std::array<char const*, NUMARGS(__VA_ARGS__)> arg_names = { MAP(STRINGCDR, __VA_ARGS__) };\
	std::tuple<MAP(APPLYCAR,__VA_ARGS__)> arg_vals;\
	CONCAT(NAME,_log)(MAP(APPLYWRITE, __VA_ARGS__)): arg_vals({MAP(APPLYCDR, __VA_ARGS__)}) {}\
};

#include "loglist.h"
#endif
