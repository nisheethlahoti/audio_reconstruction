#include <soundrex/unix/ffmpeg.h>
#include <soundrex/util.h>
#include <unistd.h>

int main(int argc, char **argv) {
	char const *vals[ffmpeg_init.size() + ffmpeg_params.size() + 1 + argc];
	constexpr std::array<char const *, 2> ffend{{"-", nullptr}};
	copy_all<char const *>(vals, {ffmpeg_init, {argv + 1, argv + argc}, ffmpeg_params, ffend});
	execvp("ffmpeg", const_cast<char **>(vals));
	perror(argv[0]);
	return 1;
}
