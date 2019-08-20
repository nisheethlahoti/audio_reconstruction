#include <cstring>
#include <iostream>

#include <soundrex/unix/lib.hpp>
#include "capture.hpp"

int capture_t::fd() const { return fd_; }
char const *capture_t::name() const { return name_; }

int capture_t::getrecv() {
	auto ret = recv;
	recv = 0;
	return ret;
}

capture_t::capture_t(char const *iface) try {
	char errbuf[PCAP_ERRBUF_SIZE];
	std::clog << "Opening interface " << iface << std::endl;
	std::strcpy(name_, iface);

	pcap = pcap_create(iface, errbuf);
	if (pcap == nullptr)
		throw std::runtime_error(std::string("unable to open: ") + errbuf);

	if (int ret = pcap_can_set_rfmon(pcap); ret != 1)
		throw std::pair(pcap, ret);

	pcap_set_snaplen(pcap, sizeof(packet_t) + 100);
	pcap_set_immediate_mode(pcap, 1);
	pcap_set_rfmon(pcap, 1);

	if (int ret = pcap_activate(pcap); ret < 0)
		throw std::pair(pcap, ret);
	else if (ret > 0)
		std::clog << "Warning activating: " << pcap_statustostr(ret) << std::endl;

	fd_ = pcap_get_selectable_fd(pcap);
	if (fd_ == -1)
		throw std::pair(pcap, -1);
} catch (std::pair<pcap_t *, int> const &err) {
	auto str = std::string(pcap_statustostr(err.second)) + ": " + pcap_geterr(err.first);
	pcap_close(err.first);
	throw std::runtime_error(str);
}

capture_t::capture_t(capture_t &&other) {
	std::memcpy(this, &other, sizeof(*this));
	other.pcap = nullptr;
}

std::span<unsigned char const> capture_t::get_packet() {
	pcap_pkthdr *header;
	u_char const *packet;
	int ret = pcap_next_ex(pcap, &header, &packet);
	if (ret == -1)
		throw std::runtime_error(std::string("packet sniffing: ") + pcap_geterr(pcap));

	++recv;
	uint32_t header_len = 32u /*mac*/ + (packet[2] | uint32_t(packet[3]) << 8) /*radiotap*/;
	return {packet + header_len, packet + header->caplen};
}

void capture_t::inject(std::span<uint8_t> packet) {
	if (pcap_sendpacket(pcap, packet.data(), packet.size()) != 0)
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

std::vector<capture_t> open_captures(std::span<char const *const> names) {
	std::vector<capture_t> captures;
	captures.reserve(names.size());
	for (char const *name : names)
		trap_error([&captures, name]() { captures.emplace_back(name); });

	std::clog << captures.size() << " interfaces opened for capture." << std::endl;
	return captures;
}
