// Callbacks that must be defined by the platform

#include <receiver/logtypes.h>
#include <receiver/constants.h>

void write_samples(sample_t const *samples, size_t len);

template <class log_t>
void packet_log(log_t log);

template <class log_t>
void play_log(log_t log);
