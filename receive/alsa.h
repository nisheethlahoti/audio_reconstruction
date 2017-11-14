#include <alsa/asoundlib.h>
#include <chrono>
#include <cstdint>
#include <thread>

#include "receive.h"

constexpr std::chrono::microseconds duration(packet_samples * 1000000ull / samples_per_s);
char const device[] = "default";			/* playback device */

extern snd_pcm_t *handle;
void init_pcm();
