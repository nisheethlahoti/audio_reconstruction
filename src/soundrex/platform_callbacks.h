// Callbacks that must be defined by the platform

#include <soundrex/constants.h>
#include <soundrex/logtypes.h>
#include <queue>

extern uint32_t nominal;
extern std::queue<packet_t> packets;

void write_samples(sample_t const* samples, size_t len);
void white_noise(packet_t const* first, packet_t const* second);
void smooth_merge(packet_t const* first, packet_t const* second);
void silence(packet_t const* first, packet_t const* second);
void sharp_merge(packet_t const* first, packet_t const* second);
void reconstruct(packet_t const* first, packet_t const* second);

template <class log_t>
void packet_log(log_t log);

template <class log_t>
void play_log(log_t log);
