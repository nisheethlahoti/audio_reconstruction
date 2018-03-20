#ifndef SOUNDREX_UTIL_H
#define SOUNDREX_UTIL_H

#include <algorithm>
#include <initializer_list>

// To be replaced by std::span when that matures.
template <typename T>
class slice_t {
	T const *begin_, *end_;

   public:
	slice_t(T const *begin, T const *end) : begin_(begin), end_(end) {}

	template <typename Container>
	slice_t(Container const &ctn) : begin_(ctn.begin()), end_(ctn.end()) {}

	T const *data() const noexcept { return begin_; }
	ssize_t size() const noexcept { return end_ - begin_; }
	T const *begin() const noexcept { return begin_; }
	T const *end() const noexcept { return end_; }
};

template <typename T, typename Itr>
constexpr Itr copy_all(Itr itr, std::initializer_list<slice_t<T>> slices) {
	for (auto const &slice : slices)
		itr = std::copy(slice.begin(), slice.end(), itr);
	return itr;
}

#endif
