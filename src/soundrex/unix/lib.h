#include <soundrex/constants.h>
#include <soundrex/util.h>
#include <functional>
#include <system_error>

void trap_error(std::function<void()> func);

template <typename T>
T wrap_error(T inp, char const *descr, T guard = -1) noexcept(false) {
	if (inp == guard)
		throw std::system_error(errno, std::system_category(), descr);
	return inp;
}
