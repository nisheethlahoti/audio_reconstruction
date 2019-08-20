#ifndef SOUNDREX_UTIL_H
#define SOUNDREX_UTIL_H

#include <algorithm>
#include <initializer_list>
#include <span>

template <typename T, typename Itr>
constexpr Itr copy_all(Itr itr, std::initializer_list<std::span<T>> slices) {
	for (auto const &slice : slices)
		itr = std::copy(slice.begin(), slice.end(), itr);
	return itr;
}

#endif
