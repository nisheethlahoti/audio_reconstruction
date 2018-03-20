#include <soundrex/util.h>
#include <unistd.h>
#include <unix/soundrex/ffmpeg.h>

int main(int argc, char **argv) {
	std::array<char const *, 2> const in{{"-i", "-"}};
	char const *arr[ffmpeg_init.size() + ffmpeg_params.size() + 2 + argc];
	*copy_all<char const *>(arr, {ffmpeg_init, ffmpeg_params, in, {argv + 1, argv + argc}}) = NULL;
	execvp("ffmpeg", (char **)arr);
	perror("executing ffmpeg");
}
