#include <iostream>
#include "lib.h"

void trap_error(std::function<void()> func) try { func(); } catch (std::runtime_error &err) {
	std::cerr << err.what() << '\n';
}
