#include <soundrex/util.h>
#include <unistd.h>
#include <unix/soundrex/ffmpeg.h>

int main(int argc, char **argv) {
	char const *vals[ffmpeg_init.size() + ffmpeg_params.size() + 1 + argc];
	constexpr std::array<char const *, 2> ffend{{"-", nullptr}};
	copy_all<char const *>(vals, {ffmpeg_init, {argv + 1, argv + argc}, ffmpeg_params, ffend});
	execvp("ffmpeg", (char **)vals);
	perror(argv[0]);
	return 1;
}
