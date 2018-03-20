#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "common.h"

void send_stdin() {
	auto resolved = wrap_error<char *>(realpath("/dev/stdin", nullptr), "realpath(cin)", nullptr);
	int fd = wrap_error(open("/dev/null", 0), "opening /dev/null");
	wrap_error(dup2(fd, 0), "redirecting input to /dev/null");
	wrap_error(close(fd), "closing additional file pointer to /dev/null");
	std::cout << resolved << std::endl;
	free(resolved);
}

void trap_error(std::function<void()> func) try { func(); } catch (std::runtime_error &err) {
	std::cerr << err.what() << '\n';
}
