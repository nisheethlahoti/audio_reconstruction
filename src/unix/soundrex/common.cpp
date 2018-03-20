#include <iostream>
#include "common.h"

void send_stdin() {
	auto resolved = wrap_error<char *>(realpath("/dev/stdin", nullptr), "realpath(cin)", nullptr);
	std::cout << resolved << std::endl;
	free(resolved);
}

void trap_error(std::function<void()> func) try { func(); } catch (std::runtime_error &err) {
	std::cerr << err.what() << '\n';
}
