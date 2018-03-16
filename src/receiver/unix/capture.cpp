#include <cstdio>
#include <cstring>
#include <iostream>

#include <receiver/unix/capture.h>

int capture_t::fd() const { return fd_; }
char const *capture_t::name() const { return name_; }
void capture_t::addrecv() { ++recv; }

unsigned capture_t::getrecv() {
	auto ret = recv;
	recv = 0;
	return ret;
}

capture_t::capture_t(char const *iface) : recv(0) {
	char errbuf[PCAP_ERRBUF_SIZE];
	std::cerr << "Opening interface " << iface << std::endl;
	fd_ = -1;
	std::strcpy(name_, iface);

	pcap = pcap_create(iface, errbuf);
	if (pcap == nullptr) {
		std::cerr << "unable to open: " << errbuf << std::endl;
		return;
	}

	if (pcap_can_set_rfmon(pcap) != 1) {
		std::cerr << "Can't set monitor mode on " << iface << std::endl;
		return;
	}

	pcap_set_snaplen(pcap, packet_size + 100);
	pcap_set_immediate_mode(pcap, 1);
	pcap_set_rfmon(pcap, 1);
	pcap_set_timeout(pcap, -1);

	if (int ret = pcap_activate(pcap); ret) {
		std::cerr << "Error activating: " << pcap_statustostr(ret) << std::endl;
		pcap_perror(pcap, "Activation error detail");
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

slice_t capture_t::get_packet() {
	pcap_pkthdr header;
	uint8_t const *packet = pcap_next(pcap, &header);
	if (packet == nullptr) {
		perror("unable to obtain packet");
		exit(1);
	}

	uint32_t header_len = 32u /*mac*/ + (packet[2] | uint32_t(packet[3]) << 8) /*radiotap*/;
	return slice_t{packet + header_len, ssize_t(header.caplen) - ssize_t(header_len)};
}

capture_t::~capture_t() {
	if (pcap != nullptr)
		pcap_close(pcap);
}
