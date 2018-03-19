#include <unix/soundrex/capture.h>
#include <cstdio>
#include <cstring>
#include <iostream>

int capture_t::fd() const { return fd_; }
char const *capture_t::name() const { return name_; }

int capture_t::getrecv() {
	auto ret = recv;
	recv = 0;
	return ret;
}

capture_t::capture_t(char const *iface) {
	char errbuf[PCAP_ERRBUF_SIZE];
	std::cerr << "Opening interface " << iface << std::endl;
	std::strcpy(name_, iface);

	pcap = pcap_create(iface, errbuf);
	if (pcap == nullptr) {
		std::cerr << "unable to open: " << errbuf << std::endl;
	} else {
		try {
			if (int ret = pcap_can_set_rfmon(pcap); ret != 1)
				throw std::pair("setting monitor mode", ret);

			pcap_set_snaplen(pcap, sizeof(packet_t) + 100);
			pcap_set_immediate_mode(pcap, 1);
			pcap_set_rfmon(pcap, 1);

			if (int ret = pcap_activate(pcap); ret < 0)
				throw std::pair("activating", ret);
			else if (ret > 0)
				std::cerr << "Warning activating: " << pcap_statustostr(ret) << std::endl;

			fd_ = pcap_get_selectable_fd(pcap);
			if (fd_ == -1)
				std::cerr << "Error getting fd_ from pcap\n";
		} catch (std::pair<char const *, int> err) {
			std::cerr << "Error " << err.first << ": " << pcap_statustostr(err.second) << std::endl;
			pcap_perror(pcap, "Detail");
		}
	}
}

capture_t::capture_t(capture_t &&other) {
	std::memcpy(this, &other, sizeof(*this));
	other.pcap = nullptr;
}

slice_t capture_t::get_packet() {
	pcap_pkthdr *header;
	u_char const *packet;
	int ret = pcap_next_ex(pcap, &header, &packet);
	if (ret == -1)
		throw std::runtime_error(std::string("packet sniffing: ") + pcap_geterr(pcap));

	++recv;
	uint32_t header_len = 32u /*mac*/ + (packet[2] | uint32_t(packet[3]) << 8) /*radiotap*/;
	return slice_t{packet + header_len, ssize_t(header->caplen) - ssize_t(header_len)};
}

void capture_t::inject(slice_t packet) {
	if (pcap_sendpacket(pcap, packet.data, packet.size) != 0)
		throw std::runtime_error(std::string("packet injection: ") + pcap_geterr(pcap));
}

void capture_t::setfilter(char const *filter) {
	bpf_program bpf;
	if (pcap_compile(pcap, &bpf, filter, 1, PCAP_NETMASK_UNKNOWN) || pcap_setfilter(pcap, &bpf))
		throw std::runtime_error(std::string("setting filter: ") + pcap_geterr(pcap));
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
