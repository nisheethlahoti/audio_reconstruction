#include <soundrex/unix/ffmpeg.h>
#include <soundrex/util.h>
#include <unistd.h>

int main(int argc, char **argv) {
	std::array<char const *, 2> const in{{"-i", "-"}};
	char const *arr[ffmpeg_init.size() + ffmpeg_params.size() + 2 + argc];
	*copy_all<char const *>(arr, {ffmpeg_init, ffmpeg_params, in, {argv + 1, argv + argc}}) = NULL;
	execvp("ffmpeg", const_cast<char **>(arr));
	perror(argv[0]);
	return 1;
}
