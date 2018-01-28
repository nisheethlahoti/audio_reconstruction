#include <fstream>
#include <iostream>

#include "../src/logger.h"

using namespace std;

template <class logtype>
void read_log(std::istream &in, logtype &log_m) {
	for_each(log_m.arg_vals, [&in, &log_m](int index, auto &val) {
		in.read(reinterpret_cast<char *>(&val), sizeof(val));
	});
}

#define ZERO_OUT(X) 0
#undef LOG_TYPE
#define LOG_TYPE(ID, NAME, ...)                             \
	case ID: {                                              \
		CONCAT(NAME, _log) val{MAP(ZERO_OUT, __VA_ARGS__)}; \
		read_log(in, val);                                  \
		text_log(cout, timediff, val);                      \
		break;                                              \
	}

istream &read_file(istream &in) {
	static uint64_t curr_time = 0;
	uint32_t timediff = static_cast<uint8_t>(in.get());
	if (timediff & 1)
		in.read(reinterpret_cast<char *>(&timediff) + 1, 3);
	else
		in.read(reinterpret_cast<char *>(&timediff) + 1, 1);
	if (!(timediff + 1)) {
		cout << "Wrong time: ";
	}
	timediff >>= 1;
	curr_time += timediff;
	cout << curr_time << ' ';
	switch (in.get()) {
#include "../src/loglist.h"
	}
	return in;
}

int main(int argc, char *argv[]) {
	ifstream infile(argv[1]);
	while (read_file(infile)) {
	}
	infile.close();
	return 0;
}
