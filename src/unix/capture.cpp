#include <cstdio>
#include <cstring>
#include <iostream>

#include <unix/capture.h>

int capture_t::fd() const { return fd_; }
char const *capture_t::name() const { return name_; }

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

	pcap_set_snaplen(pcap, sizeof(packet_t) + 100);
	pcap_set_immediate_mode(pcap, 1);
	pcap_set_rfmon(pcap, 1);

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
	pcap_pkthdr *header;
	u_char const *packet;
	int ret = pcap_next_ex(pcap, &header, &packet);
	if (ret == -1) {
		pcap_perror(pcap, "unable to obtain packet");
		exit(1);
	}

	++recv;
	uint32_t header_len = 32u /*mac*/ + (packet[2] | uint32_t(packet[3]) << 8) /*radiotap*/;
	return slice_t{packet + header_len, ssize_t(header->caplen) - ssize_t(header_len)};
}

void capture_t::inject(slice_t packet) {
	if (pcap_sendpacket(pcap, packet.data, packet.size) != 0)
		pcap_perror(pcap, "Failed to inject song packet");
}

void capture_t::setfilter(char const *filter) {
	bpf_program bpf;
	if (pcap_compile(pcap, &bpf, filter, 1, PCAP_NETMASK_UNKNOWN) || pcap_setfilter(pcap, &bpf)) {
		pcap_perror(pcap, "Error setting filter");
		exit(1);
	}
}

capture_t::~capture_t() {
	if (pcap != nullptr)
		pcap_close(pcap);
}

std::vector<capture_t> open_captures(int num, char **names) {
	std::vector<capture_t> captures;
	captures.reserve(num);
	for (int i = 0; i < num; ++i) {
		captures.emplace_back(names[i]);
		if (captures.back().fd() == -1)
			captures.pop_back();
	}

	std::cerr << captures.size() << " interfaces opened for capture." << std::endl;
	return captures;
}
