#include <cstdio>
#include <cstring>
#include <iostream>

#include "capture.h"

int capture_t::fd() const { return fd_; }
std::string const &capture_t::name() const { return name_; }
void capture_t::addrecv() { ++recv; }

unsigned capture_t::getrecv() {
	auto ret = recv;
	recv = 0;
	return ret;
}

capture_t::capture_t(char const *iface) : name_(iface), recv(0) {
	char errbuf[PCAP_ERRBUF_SIZE];
	std::cerr << "Opening interface " << iface << std::endl;
	fd_ = -1;

	pcap = pcap_open_live(iface, packet_size + 30, 1, -1, errbuf);
	if (pcap == nullptr) {
		std::cerr << "unable to open: " << errbuf << std::endl;
		return;
	}

	fd_ = pcap_get_selectable_fd(pcap);
	if (fd_ == -1)
		perror("Error getting fd_ from pcap");
}

capture_t::capture_t(capture_t &&other) {
	std::memcpy(this, &other, sizeof(*this));
	other.pcap = nullptr;
}

raw_packet_t capture_t::get_packet() {
	pcap_pkthdr header;
	uint8_t const *packet = pcap_next(pcap, &header);
	if (packet == nullptr) {
		perror("unable to obtain packet");
		exit(1);
	}

	size_t radiotap_len = packet[2] | size_t(packet[3]) << 8;
	return raw_packet_t{packet + radiotap_len, header.caplen - radiotap_len};
}

capture_t::~capture_t() {
	if (pcap)
		pcap_close(pcap);
}