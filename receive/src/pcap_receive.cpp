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
ofstream logfile("log.bin");

void my_callback(u_char *trash, pcap_pkthdr const *pkthdr,
                 u_char const *packet) {
	size_t radiotap_len = packet[2] | size_t(packet[3]) << 8;
	receive_callback(packet + radiotap_len, pkthdr->caplen - radiotap_len);
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

	/**
	 * Finally, we have the packet and are ready to inject it.
	 * First, we open the interface we want to inject on using pcap.
	 */

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
	thread(playing_loop, chrono::steady_clock::now()).detach();
	thread t(pcap_loop, ppcap2, -1, my_callback, nullptr);
	pcap_loop(ppcap, -1, my_callback, nullptr);
	return 0;
}
