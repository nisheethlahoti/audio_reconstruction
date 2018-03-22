#include <soundrex/constants.h>

constexpr std::array<char const *, 4> ffmpeg_init{{"ffmpeg", "-hide_banner", "-v", "error"}};

auto const ffmpeg_params = []() -> auto {
	static auto const rate = std::to_string(samples_per_s), ch = std::to_string(num_channels);
	static auto const fmt = "s" + std::to_string(8 * byte_depth) + (byte_depth > 1 ? "le" : "");
	return std::array<char const *, 6>{{"-f", fmt.c_str(), "-ar", rate.c_str(), "-ac", ch.c_str()}};
}
();
