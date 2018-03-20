#include <fcntl.h>
#include <soundrex/constants.h>
#include <unistd.h>
#include <unix/soundrex/common.h>
#include <iostream>
#include <string>

int main(int argc, char **argv) try {
	if (argc < 2)
		throw std::runtime_error(std::string("Usage: ") + argv[0] + "<music_file>");

	send_stdin();
	int pipes[2];
	wrap_error(pipe2(pipes, O_CLOEXEC), "creating pipes");
	wrap_error(dup2(pipes[0], 0), "duplicating stdin");

	if (!wrap_error(fork(), "forking"))
		wrap_error(execlp("./throttle", argv[0], nullptr), "opening throttle");

	auto const rate = std::to_string(samples_per_s), numch = std::to_string(num_channels);
	auto const format = "s" + std::to_string(8 * byte_depth) + (byte_depth > 1 ? "le" : "");

	wrap_error(dup2(open("/dev/null", O_CLOEXEC), 0), "opening /dev/null");
	wrap_error(dup2(pipes[1], 1), "duplicating stdout");
	wrap_error(execlp("ffmpeg", argv[0], "-hide_banner", "-loglevel", "error", "-i", argv[1], "-f",
	                  format.c_str(), "-ar", rate.c_str(), "-ac", numch.c_str(), "-", nullptr),
	           "Opening ffmpeg");
} catch (std::runtime_error const &err) {
	std::cerr << argv[0] << ": " << err.what() << '\n';
	return 1;
}
