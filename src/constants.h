#ifndef SOUNDREX_CONSTANTS
#define SOUNDREX_CONSTANTS

#include <array>
#include <chrono>

std::array<uint8_t, 4> const magic_number = {{0x16, 0xb9, 0x72, 0x99}};
std::array<uint8_t, 4> const uid = {};

constexpr size_t packet_samples = 240, trailing_samples = packet_samples / 2;
constexpr uint32_t samples_per_s = 48000;
constexpr size_t byte_depth = 2, num_channels = 1;

constexpr size_t total_samples = packet_samples + trailing_samples;
constexpr size_t packet_size = byte_depth * num_channels * total_samples + 12;
constexpr std::chrono::microseconds duration(packet_samples * 1000000ull / samples_per_s);

typedef std::array<uint8_t, byte_depth> mono_sample_t;
typedef std::array<mono_sample_t, num_channels> sample_t;

// Where negative size values represent error condition
struct slice_t {
	uint8_t const *data;
	ssize_t size;
};

#endif
