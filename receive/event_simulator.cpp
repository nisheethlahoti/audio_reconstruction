#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "pcap_file.h"
#include "wav_file.h"
#include "receive.h"

using namespace std;
wav_t wav;

uint64_t increment_micros_100 = 100000000ULL / samples_per_s;
void send_sample(sample_t sample) {
	wav.samples.push_back(sample.val ^ 1U<<15);
}

int main(int argc, char* argv[]) {
	pcap_file pcap_f(argv[1]);
	uint64_t current_time = (pcap_f.packets[0].time()-100000)*100;

	for (pcap_packet_t &packet: pcap_f.packets) {
		while (current_time < packet.time()*100) {
			current_time += increment_micros_100;
			timer_callback();
		}
		receive_callback(packet.packet_pos(), packet.packet_len());
	}

	wav.write_to_file("output.wav");
	return 0;
}
