#include <system_error>

// Sets unbuffered operations on stdin and stdout, and FIFO scheduling on the current thread
void set_realtime();

// For processes that want to send their stdin over their stdout
void send_stdin() noexcept(false);

template <typename T>
T wrap_error(T inp, char const *descr, T guard = -1) noexcept(false) {
	if (inp == guard)
		throw std::system_error(errno, std::system_category(), descr);
	return inp;
}
