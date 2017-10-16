#include <array>
std::array<uint8_t, 4> const magic_number = {0x16, 0xb9, 0x72, 0x99};
std::array<uint8_t, 4> const uid = {};
constexpr size_t packet_samples = 48;
constexpr uint32_t samples_per_s = 48000;
constexpr size_t byte_depth = 3, num_channels = 2;
constexpr int max_redundancy = 20;
