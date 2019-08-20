#include <iostream>

#include <soundrex/logtypes.hpp>

using namespace std;

template <class logtype>
void read_log(std::istream &cin, logtype &log_m) {
	for_each(log_m.arg_vals,
	         [&cin](int, auto &val) { cin.read(reinterpret_cast<char *>(&val), sizeof(val)); });
}

template <class logtype>
void text_log(std::ostream &out, uint32_t timediff, logtype const &log_m) {
	out << timediff << ' ' << log_m.message << "::";
	for_each(log_m.arg_vals, [&out, &log_m](int index, auto const &val) {
		out << ' ' << log_m.arg_names[index] << ": " << val << ";";
	});
	out << std::endl;
}

#define ZERO_OUT(X) 0
#undef LOG_TYPE
#define LOG_TYPE(ID, NAME, ...)                             \
	case ID: {                                              \
		CONCAT(NAME, _log) val{MAP(ZERO_OUT, __VA_ARGS__)}; \
		read_log(cin, val);                                 \
		text_log(cout, timediff, val);                      \
		break;                                              \
	}

int main() {
	uint64_t curr_time = 0;
	cin.read(reinterpret_cast<char *>(&curr_time), 8);

	uint32_t timediff;
	while (timediff = cin.get(), cin) {
		cin.read(reinterpret_cast<char *>(&timediff) + 1, timediff & 1 ? 3 : 1);
		if (~timediff == 0) {
			cout << "Wrong time: ";
		}

		timediff >>= 1;
		curr_time += timediff;
		cout << curr_time << ' ';

		switch (cin.get()) {
#include <soundrex/logtypes.list>
			default:
				cout << "Unrecognized log id." << endl;
				return 1;
		}
	}
}
