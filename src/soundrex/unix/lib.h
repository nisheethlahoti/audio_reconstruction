#include <soundrex/constants.h>
#include <soundrex/util.h>
#include <functional>
#include <system_error>

void trap_error(std::function<void()> func);
void wait_for_input() noexcept(false);

// All of these return the number of bytes written/read/drained from stdout/stdin
size_t buf_write(void const *buf, size_t count) noexcept(false);
size_t buf_read_available(void *buf, size_t count) noexcept(false);
size_t buf_drain() noexcept(false);

// Blocks until it's read count bytes, returns false on EOF and true on read successful
bool buf_read_blocking(void *buf, size_t count) noexcept(false);  

template <typename T>
T wrap_error(T inp, char const *descr, T guard = -1) noexcept(false) {
	if (inp == guard)
		throw std::system_error(errno, std::system_category(), descr);
	return inp;
}
