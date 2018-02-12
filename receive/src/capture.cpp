#include <cstdio>
#include <cstring>
#include <iostream>

#include "capture.h"

capture_t::capture_t(char const *iface) {
	char errbuf[PCAP_ERRBUF_SIZE];
	std::cerr << "Opening interface " << iface << std::endl;
	fd_ = -1;

	pcap = pcap_open_live(iface, 2048, 1, -1, errbuf);
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

int capture_t::fd() const { return fd_; }

raw_packet_t capture_t::get_packet() const {
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
