#include <pcap.h>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <thread>
#include <unistd.h>

#include "receive.h"
#include "alsa.h"

using namespace std;

void my_callback(u_char *trash, pcap_pkthdr const *pkthdr, u_char const *packet) {
  size_t radiotap_len = packet[2] | size_t(packet[3])<<8;
  receive_callback(packet + radiotap_len, pkthdr->caplen - radiotap_len);
}

int main(int argc, char **argv) {
  ios::sync_with_stdio(false);
  //assert(sizeof dac_sample == byte_depth * num_channels);

  if (argc < 3) {
    printf("Not enough arguments\n");
    return 0;
  }

  /* PCAP vars */
  char errbuf[PCAP_ERRBUF_SIZE];

  /**
   * Finally, we have the packet and are ready to inject it.
   * First, we open the interface we want to inject on using pcap.
   */
  pcap_t *ppcap = pcap_open_live(argv[1], 2048, 1, -1, errbuf);
  pcap_t *ppcap2 = pcap_open_live(argv[2], 2048, 1, -1, errbuf);

  if (ppcap == NULL) {
    printf("Could not open interface wlan1 for packet injection: %s", errbuf);
    return 2;
  }

  init_pcm();
  thread t(pcap_loop, ppcap2, -1, my_callback, nullptr);
  pcap_loop(ppcap, -1, my_callback, nullptr);
  return 0;
}
