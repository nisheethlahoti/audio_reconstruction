#ifndef SOUNDREX_CONSTANTS
#define SOUNDREX_CONSTANTS

#include <array>
#include <chrono>

constexpr std::array<uint8_t, 6> pin = {{0xc3, 0x24, 0x80, 0x30, 0xcd, 0xa8}};
constexpr size_t header_size = 4;  // contains only packet number now

constexpr size_t packet_samples = 240, trailing_samples = packet_samples / 2;
constexpr uint32_t samples_per_s = 48000;
constexpr size_t byte_depth = 2, num_channels = 1;

constexpr size_t total_samples = packet_samples + trailing_samples;
constexpr std::chrono::microseconds duration(packet_samples * 1000000ull / samples_per_s);

typedef std::array<uint8_t, byte_depth> mono_sample_t;
typedef std::array<mono_sample_t, num_channels> sample_t;

struct packet_t {
	uint32_t num;
	std::array<sample_t, packet_samples> samples;
	std::array<sample_t, trailing_samples> trailing;
};

// Where negative size values represent error condition
struct slice_t {
	uint8_t const *data;
	ssize_t size;
};

static_assert(sizeof(packet_t) == header_size + byte_depth * num_channels * total_samples);

#endif
