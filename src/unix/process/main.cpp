#include <signal.h>
#include <soundrex/processor.h>
#include <unix/soundrex/common.h>
#include <iostream>
#include <thread>

static processor_t processor;

static void toggle_corrections(int sigtype) {
	std::cerr << (processor.toggle_corrections() ? "Correcting\n" : "Not correcting\n");
}

static void playing_loop() {
	auto time = std::chrono::steady_clock::now();
	while (std::cin) {
		std::this_thread::sleep_until(time += duration);
		processor.play_next();
	}
}

int main() {
	set_realtime();

	struct sigaction action {};
	action.sa_handler = toggle_corrections;
	action.sa_flags = SA_RESTART;
	sigaction(SIGURG, &action, nullptr);  // Toggles on receiving SIGURG

	std::thread player(playing_loop);
	std::array<uint8_t, sizeof(packet_t)> buf;
	while (std::cin.read(reinterpret_cast<char*>(buf.data()), buf.size()))
		processor.process(buf.data());

	player.join();
}
