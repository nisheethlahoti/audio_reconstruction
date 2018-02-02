#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <pcap.h>
#include <signal.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#include "receive.h"

using namespace std;

void my_callback(u_char *logptr, pcap_pkthdr const *pkthdr,
                 u_char const *packet) {
	size_t radiotap_len = packet[2] | size_t(packet[3]) << 8;
	receive_callback(packet + radiotap_len, pkthdr->caplen - radiotap_len, *reinterpret_cast<logger_t*>(logptr));
}

int main(int argc, char **argv) {
	signal(SIGINT, exit);

	ios::sync_with_stdio(false);
	// assert(sizeof dac_sample == byte_depth * num_channels);

	if (argc < 3) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	/* PCAP vars */
	char errbuf[PCAP_ERRBUF_SIZE];

	pcap_t *ppcap = pcap_open_live(argv[1], 2048, 1, -1, errbuf);
	if (!ppcap) {
		cerr << argv[1] << ": unable to open: " << errbuf << endl;
		return 2;
	}

	pcap_t *ppcap2 = pcap_open_live(argv[2], 2048, 1, -1, errbuf);
	if (!ppcap2) {
		cerr << argv[2] << ": unable to open: " << errbuf << endl;
		return 2;
	}

	initialize_player();

	logger_t playlogger("play.bin"), if1logger("if1.bin"), if2logger("if2.bin");
	thread(playing_loop, std::ref(playlogger)).detach();
	thread t(pcap_loop, ppcap2, -1, my_callback, reinterpret_cast<u_char*>(&if2logger));
	pcap_loop(ppcap, -1, my_callback, reinterpret_cast<u_char*>(&if1logger));
	return 0;
}
