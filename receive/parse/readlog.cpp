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
#define LOG_TYPE(ID, NAME, ...)                               \
	case ID: {                                                \
		CONCAT(NAME, _log)                                    \
		val = CONCAT(NAME, _log)(MAP(ZERO_OUT, __VA_ARGS__)); \
		read_log(in, val);                                    \
		text_log(cout, val);                                  \
		break;                                                \
	}

istream &read_file(istream &in) {
	switch (in.get()) {
#include "../src/loglist.h"
	}
}

int main(int argc, char *argv[]) {
	ifstream infile(argv[1]);
	while (infile) {
		read_file(infile);
	}
	infile.close();
	return 0;
}
