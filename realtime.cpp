#include <iostream>
#include <pthread.h>
#include <sched.h>

#include "realtime.h"

void set_realtime(int const weight_max, int const weight_min) {
	constexpr auto policy = SCHED_FIFO;
	int const maxpr = sched_get_priority_max(policy), minpr = sched_get_priority_min(policy);
	sched_param const param{(weight_max * maxpr + weight_min * minpr) / (weight_max + weight_min)};
	pthread_setschedparam(pthread_self(), policy, &param);
	std::cin.setf(std::ios::unitbuf);
	std::cout.setf(std::ios::unitbuf);
}
