#include <iostream>

#include "../src/logger.h"

using namespace std;

template <class logtype>
void read_log(std::istream &in, logtype &log_m) {
	for_each(log_m.arg_vals, [&in, &log_m](int index, auto &val) {
		in.read(reinterpret_cast<char *>(&val), sizeof(val));
	});
}

template <class logtype>
void partial_log(std::ostream &out, uint32_t timediff, logtype const &log_m) {
	out << timediff << ' ' << log_m.message << "::";
	for_each(log_m.arg_vals, [&out, &log_m](int index, auto const &val) {
		out << ' ' << log_m.arg_names[index] << ": " << val << ";";
	});
	out << std::endl;
}

void bytes_log(std::istream &in, std::ostream &out, int size) {
	char bytes[size];
	in.read(bytes, sizeof(bytes));
	for (int i = 0; i < sizeof(bytes); ++i) {
		out << uint16_t(uint8_t(bytes[i])) << ' ';
	}
	out << std::endl;
}

template <class logtype>
void text_log(std::istream &in, std::ostream &out, uint32_t timediff, logtype const &log_m) {
	partial_log(out, timediff, log_m);
}

template <>
void text_log(std::istream &in, std::ostream &out, uint32_t timediff, captured_log const &log_m) {
	partial_log(out, timediff, log_m);
	bytes_log(in, out, std::get<1>(log_m.arg_vals));
}

template <>
void text_log(std::istream &in, std::ostream &out, uint32_t timediff, validated_log const &log_m) {
	partial_log(out, timediff, log_m);
	bytes_log(in, out, std::get<0>(log_m.arg_vals));
}

#define ZERO_OUT(X) 0
#undef LOG_TYPE
#define LOG_TYPE(ID, NAME, ...)                             \
	case ID: {                                              \
		CONCAT(NAME, _log) val{MAP(ZERO_OUT, __VA_ARGS__)}; \
		read_log(cin, val);                                 \
		text_log(cin, cout, timediff, val);                 \
		break;                                              \
	}

int main() {
	uint64_t curr_time = 0;
	cin.read(reinterpret_cast<char *>(&curr_time), 8);

	uint32_t timediff;
	while ((timediff = cin.get()) != istream::traits_type::eof()) {
		cin.read(reinterpret_cast<char *>(&timediff) + 1, timediff & 1 ? 3 : 1);
		if (~timediff == 0) {
			cout << "Wrong time: ";
		}

		timediff >>= 1;
		curr_time += timediff;
		cout << curr_time << ' ';

		switch (cin.get()) {
#include "../src/loglist.h"
		}
	}
	return 0;
}
