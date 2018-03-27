#include <fcntl.h>
#include <soundrex/unix/ffmpeg.h>
#include <soundrex/unix/lib.h>
#include <unistd.h>

int main(int argc, char **argv) try {
	if (argc != 2)
		throw std::runtime_error(std::string("Usage: ") + argv[0] + " <input>");

	wrap_error(dup2(open("/dev/null", O_RDONLY | O_CLOEXEC), 0), "Redirecting input to /dev/null");
	char const *vals[ffmpeg_init.size() + ffmpeg_params.size() + 3];
	std::array<char const *, 2> const ffend{{"-", nullptr}}, inp{{"-i", argv[1]}};
	copy_all<char const *>(vals, {ffmpeg_init, inp, ffmpeg_params, ffend});
	execvp("ffmpeg", const_cast<char **>(vals));
	wrap_error(-1, argv[0]);
} catch (std::runtime_error const &err) {
	fprintf(stderr, "%s\n", err.what());
	return 1;
}
