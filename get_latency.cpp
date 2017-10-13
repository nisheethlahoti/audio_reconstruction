#include <chrono>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace chrono;

struct tp_t {
	time_point<system_clock> val;
};

ostream& operator<<(ostream &out, tp_t tp) {
    auto ms = duration_cast<microseconds>(tp.val.time_since_epoch()) % 1000000;
    auto timer = system_clock::to_time_t(tp.val);
    tm bt = *std::localtime(&timer);

    ostringstream oss;
    oss << put_time(&bt, "%T") << '.' << setfill('0') << setw(6) << ms.count();
    return out << oss.str();
}

istream& operator>>(istream &in, tp_t &tp) {
	uint32_t micros;
	tm bt = {};
	bt.tm_year = 100;
	in >> get_time(&bt, "%T.") >> micros;
	tp.val =  system_clock::from_time_t(mktime(&bt)) + microseconds(micros);
	return in;
}

struct packet_t {
	uint16_t pnum;
	tp_t transmitted, received;
};

int main() {
	ifstream numbers("numbers.txt"), transmission("transmission.txt"), timestamps("timestamps.txt");
	vector<packet_t> packets;

	uint16_t pnum;
	tp_t tp;
	char str[5];

	while (transmission >> pnum >> tp) {
		packets.push_back({pnum, tp, tp_t()});
	}

	while (numbers >> str) {
		swap(str[0], str[2]);
		swap(str[1], str[3]);
		pnum = strtol(str, nullptr, 16);
		timestamps >> tp;
		packets[pnum].received = tp;
	}

	uint64_t sum=0, sqsum=0, num=0;
	for (packet_t &packet: packets) {
		if (packet.received.val != time_point<system_clock>()) {
			++num;
			int64_t tdiff = duration_cast<microseconds>(packet.received.val - packet.transmitted.val).count();
			cout << packet.pnum << ' ' << (double)tdiff/1000 << endl;// << ' ' << packet.transmitted << ' ' << packet.received << endl;
			sum += tdiff;
			sqsum += tdiff*tdiff;
		}
	}

	cout << "Transmitted " << packets.size() << " packets and received " << num << " of them.\n";
	cout << "Drop rate is " << 1 - (double)num/packets.size() << endl;

	if (num != 0) {
		double mean = (double)sum/1000/num;
		cout << "Mean delay is " << mean << " milliseconds.\n";
		cout << "Standard deviation in delay is " << sqrt((double)sqsum/1000000/num - mean*mean) << " milliseconds.\n";
	}

	return 0;
}
