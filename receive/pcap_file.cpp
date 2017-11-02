#include <fstream>
#include "pcap_file.h"

using namespace std;

pcap_file::pcap_file(char const *filename) {
	ifstream inp_file(filename, ios::in | ios::binary);
	inp_file.read(reinterpret_cast<char*>(&header), sizeof header);
	pcap_packet_t temp_packet;
	while (inp_file.read(reinterpret_cast<char*>(&temp_packet.header), sizeof temp_packet.header)) {
		temp_packet.data = new uint8_t[temp_packet.header.incl_len];
		inp_file.read(reinterpret_cast<char*>(temp_packet.data), temp_packet.header.incl_len);
		packets.push_back(temp_packet);
	}
	inp_file.close();
}

pcap_file::~pcap_file() {
	for (pcap_packet_t const &packet: packets) {
		delete[] packet.data;
	}
}

size_t pcap_packet_t::radiotap_length() {
	return data[2] | uint16_t(data[3])<<8;
}
