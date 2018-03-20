#include <pthread.h>
#include <sched.h>
#include <iostream>
#include "main.h"

int main(int argc, char **argv) try {
	constexpr auto policy = SCHED_FIFO;
	sched_param const param{sched_get_priority_min(policy) + 10};
	pthread_setschedparam(pthread_self(), policy, &param);
	std::ios::sync_with_stdio(false);
	std::cin.setf(std::ios::unitbuf);
	std::cout.setf(std::ios::unitbuf);

	soundrex_main({argv + 1, argv + argc});
} catch (std::runtime_error const &err) {
	std::cerr << argv[0] << ": " << err.what() << '\n';
	return 1;
}
