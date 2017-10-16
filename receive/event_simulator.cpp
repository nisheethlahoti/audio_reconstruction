#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "pcap_file.h"
#include "wav_file.h"
#include "receive.h"

using namespace std;
wav_t wav;

sample_t dac_sample;

uint64_t increment_micros_100 = 100000000ULL / samples_per_s;
void send_sample(sample_t sample) {
	dac_sample = sample;
}


int main(int argc, char* argv[]) {
	assert(sizeof dac_sample == byte_depth * num_channels);

	pcap_file pcap_f(argv[1]);
	uint64_t current_time = (pcap_f.packets[0].time()-100000)*100;

	for (pcap_packet_t &packet: pcap_f.packets) {
		while (current_time < packet.time()*100) {
			current_time += increment_micros_100;
			timer_callback();
			wav.samples.push_back(dac_sample);
		}
		receive_callback(packet.packet_pos(), packet.packet_len());
	}

	wav.write_to_file("output.wav");
	return 0;
}
