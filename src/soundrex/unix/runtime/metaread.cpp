#include <soundrex/unix/runtime/lib.h>
#include <thread>

void soundrex_main(slice_t<char *>) {
	packet_t buffer;
	std::chrono::time_point<std::chrono::steady_clock> time{};

	while (buf_read_blocking(&buffer, sizeof(buffer))) {
		time = std::max(time + duration, std::chrono::steady_clock::now());
		std::this_thread::sleep_until(time);
		buf_write(&buffer, sizeof(buffer));
	}
}
