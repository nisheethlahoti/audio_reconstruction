#include <soundrex/unix/ffmpeg.h>
#include <soundrex/unix/runtime/lib.h>
#include <unistd.h>

void soundrex_main(slice_t<char *>) {
	std::array<char const *, 2> const in{{"-", nullptr}};
	char const *arr[ffplay_init.size() + ffmpeg_params.size() + in.size()];
	copy_all<char const *>(arr, {ffplay_init, ffmpeg_params, in});
	execvp("ffplay", const_cast<char **>(arr));
	wrap_error(-1, "Couldn't play");
}
