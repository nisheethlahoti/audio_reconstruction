#include <pthread.h>
#include <sched.h>
#include <iostream>
#include "common.h"

void set_realtime() {
	constexpr auto policy = SCHED_FIFO;
	sched_param const param{sched_get_priority_min(policy) + 10};
	pthread_setschedparam(pthread_self(), policy, &param);
	std::ios::sync_with_stdio(false);
	std::cin.setf(std::ios::unitbuf);
	std::cout.setf(std::ios::unitbuf);
}

void send_stdin() {
	auto resolved = wrap_error<char *>(realpath("/dev/stdin", nullptr), "realpath(cin)", nullptr);
	std::cout << resolved << std::endl;
	free(resolved);
}
