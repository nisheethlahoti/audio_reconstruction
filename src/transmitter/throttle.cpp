#include <iostream>
#include <thread>

#include <constants.h>
#include <realtime.h>

int main() {
	set_realtime(1, 1);
	std::array<sample_t, packet_samples> buffer;
	auto time = std::chrono::steady_clock::now();
	while (std::cin.read(reinterpret_cast<char*>(&buffer), sizeof(buffer))) {
		std::this_thread::sleep_until(time += duration);
		std::cout.write(reinterpret_cast<char*>(&buffer), sizeof(buffer));
	}
}
