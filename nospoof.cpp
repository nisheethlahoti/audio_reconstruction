/**
 * Hello, and welcome to this brief, but hopefully complete, example file for
 * wireless packet injection using pcap.
 *
 * Although there are various resources for this spread on the web, it is hard
 * to find a single, cohesive piece that shows how everything fits together.
 * This file aims to give such an example, constructing a fully valid UDP packet
 * all the way from the 802.11 PHY header (through radiotap) to the data part of
 * the packet and then injecting it on a wireless interface
 *
 * Skip down a couple of lines, as the following is just headers and such that
 * we need.
 */
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include <pcap.h>
#include <unistd.h>

using namespace std;
using namespace chrono;

size_t const num_samples = 18;

static const uint8_t u8aRadiotapHeader[] = {

  0x00, 0x00, // <-- radiotap version (ignore this)
  0x18, 0x00, // <-- number of bytes in our header (count the number of "0x"s)

  /**
   * The next field is a bitmap of which options we are including.
   * The full list of which field is which option is in ieee80211_radiotap.h,
   * but I've chosen to include:
   *   0x00 0x01: timestamp
   *   0x00 0x02: flags
   *   0x00 0x03: rate
   *   0x00 0x04: channel
   *   0x80 0x00: tx flags (seems silly to have this AND flags, but oh well)
   */
  0x0f, 0x80, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <-- timestamp

  /**
   * This is the first set of flags, and we've set the bit corresponding to
   * IEEE80211_RADIOTAP_F_FCS, meaning we want the card to add a FCS at the end
   * of our buffer for us.
   */
  0x10,   // yes, there should be FCS at the end of the mac frame and it should be added by the card.

  0x18, // 0x0c <-- rate
  //0x00,0x00,0x00,0x00, //0x8c, 0x14, 0x40, 0x01, // <-- channel
  0x6c, 0x09, 0xA0, 0x00,  

  /**
   * This is the second set of flags, specifically related to transmissions. The
   * bit we've set is IEEE80211_RADIOTAP_F_TX_NOACK, which means the card won't
   * wait for an ACK for this frame, and that it won't retry if it doesn't get
   * one.
   */
  0x08, 0x00,
};

uint8_t const eff_pkt[] = {
  0x45 ,0x00 ,0x00 ,0x2d ,0x07 ,0xe5 ,0x00 ,0x00 ,0x01 ,0x11 ,0xfb ,0xb7 ,0xc0 ,0xa8 ,0x02 ,0x01
 ,0xef ,0x1c ,0x04 ,0x5e ,0xc7 ,0x7a ,0x13 ,0x8d ,0x00 ,0x19 ,0xb8 ,0x49 ,0xff ,0x03 ,0x04 ,0xcd
 ,0xcc ,0xcc ,0x3d ,0x04 ,0xcd ,0xcc ,0xcc ,0x3d ,0x04 ,0xcd ,0xcc ,0xcc ,0x3d
};

uint8_t const mac_header[] = { 
                    0x08, 0x00, //Frame Control for data frame 
                    0x01, 0x01, //Duration
          //  /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //Destination address 
                    0x01, 0x00, 0x5e, 0x1c, 0x04, 0x5e,
            /*10*/  0x6e, 0x40, 0x08, 0x49, 0x01, 0x64, //Source address - overwritten later
            /*16*/  0x6e, 0x40, 0x08, 0x49, 0x01, 0x64, //BSSID - overwritten to the same as the source address
            /*22*/  0x00, 0x00, //Seq-ctl
            
            //addr4 is not present if not WDS(bridge)                               
            //IPLLC SNAP header : next 2 bytes, SNAP field : next 6 bytes
            /*24*/  0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00,
            //Frame body starts here. contains IP packet, UDP packet
            /*32*/  
};

char const custom_header[8] = "SoundRex";

uint32_t num = 0;

void fill_packet(uint8_t *packet){
  for (int i=0; i<4; ++i)
    packet[i] = (num>>(8*i)) & 0xff;
  ++num;
}

string current_time() {
    auto now = system_clock::now();
    auto ms = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;
    auto timer = system_clock::to_time_t(now);
    tm bt = *std::localtime(&timer);

    ostringstream oss;
    oss << put_time(&bt, "%T") << '.' << setfill('0') << setw(6) << ms.count();
    return oss.str();
}

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Not enough arguments\n";
    return 0;
  }

  /* PCAP vars */
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *ppcap;

  uint8_t const header_size = sizeof u8aRadiotapHeader + sizeof mac_header + sizeof custom_header;
  uint8_t buf[header_size + 3*num_samples+1 + 4 /*FCS*/];

  memcpy(buf, u8aRadiotapHeader, sizeof u8aRadiotapHeader);
  memcpy(buf + sizeof u8aRadiotapHeader, mac_header, sizeof mac_header);
  memcpy(buf + sizeof u8aRadiotapHeader + sizeof mac_header, custom_header, sizeof custom_header);

  /**
   * Finally, we have the packet and are ready to inject it.
   * First, we open the interface we want to inject on using pcap.
   */
  ppcap = pcap_open_live(argv[1], 800, 1, 20, errbuf);

  if (ppcap == NULL) {
    cerr << "Could not open interface wlan1 for packet injection: " << errbuf;
    return 2;
  }

  auto time = chrono::steady_clock::now();
  for(uint8_t i=0; ; ++i){
    if (false) {
      this_thread::sleep_for(chrono::milliseconds(3));
      fill_packet(buf + header_size);
      if(pcap_sendpacket(ppcap, buf, header_size +  17 + 4) != 0)
        pcap_perror(ppcap, "Failed to inject effect packet");
      else
        cout << num << ' ' << current_time() << endl;
    }

    fill_packet(buf + header_size);
    if(pcap_sendpacket(ppcap, buf, sizeof buf) != 0)
      pcap_perror(ppcap, "Failed to inject song packet");
    else
      cout << num << ' ' << current_time() << endl;

    time += chrono::milliseconds(16);
    while (chrono::steady_clock::now() < time);
  }
  pcap_close(ppcap);
  return 0;
}
