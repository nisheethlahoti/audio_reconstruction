#include <signal.h>
#include <soundrex/lib/processor.h>
#include <soundrex/unix/runtime/lib.h>
#include <atomic>
#include <iostream>
#include <thread>

static processor_t processor;
static std::atomic<bool> stopped(false);

static void toggle_corrections(int) {
	std::clog << (processor.toggle_corrections() ? "Correcting" : "Not correcting") << std::endl;
}

static void playing_loop() {
	auto time = std::chrono::steady_clock::now();
	while (!stopped.load(std::memory_order_acquire)) {
		std::this_thread::sleep_until(time += duration);
		processor.play_next();
	}
}

void soundrex_main(slice_t<char *>) {
	struct sigaction action {};
	action.sa_handler = toggle_corrections;
	action.sa_flags = SA_RESTART;
	sigaction(SIGURG, &action, nullptr);  // Toggles on receiving SIGURG

	std::thread player(playing_loop);
	std::array<uint8_t, sizeof(packet_t)> buf;
	while (buf_read_blocking(buf.data(), buf.size()))
		processor.process(buf.data());

	stopped.store(true, std::memory_order_release);
	player.join();
}
