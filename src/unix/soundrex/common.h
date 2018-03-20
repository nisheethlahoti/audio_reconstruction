#include <soundrex/constants.h>
#include <soundrex/util.h>
#include <functional>
#include <system_error>

// For processes that want to send their stdin over their stdout
void send_stdin() noexcept(false);

void trap_error(std::function<void()> func);

template <typename T>
T wrap_error(T inp, char const *descr, T guard = -1) noexcept(false) {
	if (inp == guard)
		throw std::system_error(errno, std::system_category(), descr);
	return inp;
}
