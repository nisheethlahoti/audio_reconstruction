#ifndef SOUNDREX_UTIL_H
#define SOUNDREX_UTIL_H

#include <algorithm>
#include <initializer_list>

// To be replaced by std::span when that matures.
template <typename T>
class slice_t {
	T const *begin_, *end_;

   public:
	constexpr slice_t(T const *begin, T const *end) noexcept : begin_(begin), end_(end) {}

	template <typename Container>
	constexpr slice_t(Container const &ctn) : begin_(ctn.begin()), end_(ctn.end()) {}

	constexpr T const &operator[](size_t index) const noexcept { return begin_[index]; }
	constexpr slice_t<T> subspan(std::ptrdiff_t off) const noexcept { return {begin_ + off, end_}; }

	constexpr T const *data() const noexcept { return begin_; }
	constexpr std::ptrdiff_t size() const noexcept { return end_ - begin_; }
	constexpr T const *begin() const noexcept { return begin_; }
	constexpr T const *end() const noexcept { return end_; }
	constexpr bool empty() const noexcept { return size() == 0; }
};

template <typename T, typename Itr>
constexpr Itr copy_all(Itr itr, std::initializer_list<slice_t<T>> slices) {
	for (auto const &slice : slices)
		itr = std::copy(slice.begin(), slice.end(), itr);
	return itr;
}

#endif
