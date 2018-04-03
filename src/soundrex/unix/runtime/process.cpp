#include <signal.h>
#include <soundrex/lib/processor.h>
#include <soundrex/platform_callbacks.h>
#include <soundrex/unix/runtime/lib.h>
#include <atomic>
#include <iostream>

static processor_t processor;
std::queue<packet_t> packets;
uint32_t nominal = 0;

constexpr reconstruct_t get_recon(char const c) {
	switch (c) {
		case 'r':
			return reconstruct_t::reconstruct;
		case 'm':
			return reconstruct_t::sharp_merge;
		case 's':
			return reconstruct_t::smooth_merge;
		case 'w':
			return reconstruct_t::white_noise;
		case 'q':
			return reconstruct_t::silence;
		default:
			throw std::runtime_error("First arg rmswq");
	}
}

void soundrex_main(slice_t<char *> args) {
	if (args.empty())
		throw std::runtime_error(" <type>");

	processor.reconstruct.store(get_recon(args[0][0]), std::memory_order_release);
	packet_t buf;

	buf_read_blocking(&buf, sizeof(buf));
	packets.push(buf);
	processor.process(reinterpret_cast<uint8_t const *>(&buf));

	while (buf_read_blocking(&buf, sizeof(buf))) {
		packets.push(buf);
		if (!buf.invisible)
			processor.process(reinterpret_cast<uint8_t const *>(&buf));
		processor.play_next();
	}
}
