#include <pthread.h>
#include <sched.h>
#include <soundrex/realtime.h>
#include <iostream>

void set_realtime() {
	constexpr auto policy = SCHED_FIFO;
	sched_param const param{sched_get_priority_min(policy) + 10};
	pthread_setschedparam(pthread_self(), policy, &param);
	std::cin.setf(std::ios::unitbuf);
	std::cout.setf(std::ios::unitbuf);
}
