#include <pthread.h>
#include <sched.h>
#include <iostream>
#include "lib.h"

int main(int argc, char **argv) try {
	constexpr auto policy = SCHED_FIFO;
	sched_param const param{sched_get_priority_max(policy) - 10};
	pthread_setschedparam(pthread_self(), policy, &param);
	std::ios::sync_with_stdio(false);
	soundrex_main({argv + 1, argv + argc});
} catch (std::runtime_error const &err) {
	std::fprintf(stderr, "%s: %s\n", argv[0], err.what());
	return 1;
}
