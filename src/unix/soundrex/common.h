#include <soundrex/constants.h>
#include <soundrex/util.h>
#include <functional>
#include <system_error>

// Sets unbuffered operations on stdin and stdout, and FIFO scheduling on the current thread
void set_realtime();

// For processes that want to send their stdin over their stdout
void send_stdin() noexcept(false);

void trap_error(std::function<void()> func);

template <typename T>
T wrap_error(T inp, char const *descr, T guard = -1) noexcept(false) {
	if (inp == guard)
		throw std::system_error(errno, std::system_category(), descr);
	return inp;
}

constexpr std::array<char const *, 4> ffmpeg_init{{"ffmpeg", "-hide_banner", "-v", "error"}};

auto const ffmpeg_params = []() -> auto {
	static auto const rate = std::to_string(samples_per_s), ch = std::to_string(num_channels);
	static auto const fmt = "s" + std::to_string(8 * byte_depth) + (byte_depth > 1 ? "le" : "");
	return std::array<char const *, 6>{{"-f", fmt.c_str(), "-ar", rate.c_str(), "-ac", ch.c_str()}};
}
();
