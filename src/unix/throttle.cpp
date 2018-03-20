#include <soundrex/constants.h>
#include <unix/soundrex/main.h>
#include <iostream>
#include <thread>

void soundrex_main(slice_t<char *>) {
	std::array<sample_t, packet_samples> buffer;
	std::chrono::time_point<std::chrono::steady_clock> time{};

	while (std::cin.read(reinterpret_cast<char *>(&buffer), sizeof(buffer))) {
		time = std::max(time + duration, std::chrono::steady_clock::now());
		std::this_thread::sleep_until(time);
		std::cout.write(reinterpret_cast<char *>(&buffer), sizeof(buffer));
	}
}
