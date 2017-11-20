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
#include <pcap.h>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <thread>
#include <unistd.h>

using namespace std;

size_t const num_samples = 384;

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

//paradise 9 sec. Thus 24k * 9 samples = 0x34BC0UL samples = 216000 samples   
uint16_t music[] = {
};

uint8_t ip_hdr_song_data[20] = {0x45, 0x00, 0x04, 0x9d, 0, 0, 0x00, 0x00, 0x80, 0x11, 0x00, 0x00,
                                192, 168, 2, 1, 239, 28, 4, 94};  // [10] [11] is checksum
uint8_t udp_hdr_song_data[8] = {0xe0, 0xb1, 0x13, 0x8d, 0x04, 0x89, 0x00, 0x00 }; //[06] [07] is checksum;

uint8_t ip_hdr_effects_data[20] = {0x45, 0x00, 0x00, 0x2d, 0, 0, 0x00, 0x00, 0x80, 0x11, 0x00, 0x00,
                                192, 168, 2, 1, 239, 28, 4, 94};  // [10] [11] is checksum
uint8_t udp_hdr_effects_data[8] = {0xe0, 0xb1, 0x13, 0x8d, 0x00, 0x19, 0x00, 0x00 }; //[06] [07] is checksum

uint16_t inet_csum(uint8_t const buf[], size_t const hdr_len) {
  uint32_t sum = 0;
  for (int i=0; i < hdr_len; i+=2){
    sum += buf[i] | uint16_t(buf[i+1]) << 8;
  }

  while (sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);

  return(~sum);
}


int fill_beacon(uint8_t *pos) {
    uint8_t packet[128] = { 0x80, 0x08, //Frame Control   0x80 0x00 initially 
                        0x00, 0x00 , //Duration
                /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //Destination address 
                /*10*/  0x6e, 0x40, 0x08, 0x49, 0x01, 0x64, //Source address - overwritten later
                /*16*/  0x6e, 0x40, 0x08, 0x49, 0x01, 0x64, //BSSID - overwritten to the same as the source address
                /*22*/  0x40, 0x54, //Seq-ctl
                //Frame body starts here
                /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, //timestamp - the number of microseconds the AP has been active
                /*32*/  0x64, 0x00, //Beacon interval
                /*34*/  0x01, 0x00, //Capability info
                /* SSID */
                /*36*/  0x00
                };

    char ssid[13] = "soundrex_dev";
    int ssidLen = 12;
    packet[37] = ssidLen;

    for(int i = 0; i < ssidLen; i++) {
      packet[38+i] = ssid[i];
    }

    uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, //supported rate
                        0x03, 0x01, 0x04 /*DSSS (Current Channel)*/ };

    for(int i = 0; i < 12; i++) {
      packet[38 + ssidLen + i] = postSSID[i];
    }

    packet[50 + ssidLen] = 1;

    int packetSize = 51 + ssidLen;
    memcpy(pos, packet, packetSize);
    return packetSize;   
}

void fill_song_packet(uint8_t *packet){
    typedef uint16_t const * arrptr;
    static arrptr const init = music;
    static arrptr const fin = init + sizeof music / sizeof music[0];
    static arrptr curr = init;
    static int num = 0;

    memcpy(packet, ip_hdr_song_data, sizeof ip_hdr_song_data);
    memcpy(packet + sizeof ip_hdr_song_data, udp_hdr_song_data, sizeof udp_hdr_song_data);

    packet[28] = num;
    num = (num+1)%252;

//    printf("Aaaaaa\n");
    if(curr > (fin-2000)) curr = init;
    for(int i=0; i<num_samples;i++){
//      printf("Got %ld\n", curr - init);
      uint8_t val = (*curr++) >> 4 ; 
//      printf("kkk  %ld kkk %lu\n", fin-init, sizeof music/2);
      packet[3*i+29] = val;       // song_data[i];   // pgm_read_word_far(init++);
      packet[3*i+30] = val;
      packet[3*i+31] = val;
    }
//    printf("Bbbbbbb\n");

    uint16_t ip_csm = inet_csum(packet, 20);
    packet[10] = ip_csm & 0x00FF ;
    packet[11] = (ip_csm >> 8) & 0x00FF ;
    
    uint16_t udp_csm=0;
    packet[26] = udp_csm & 0x00FF ;
    packet[27] = (udp_csm >> 8) & 0x00FF ;
}

void fill_effect_packet(uint8_t *packet){
    static uint8_t const effect[] = {0xff, 0x03 , 0x04 , 0xcd , 0xcc , 0xcc , 0x3d , 0x04 , 0xcd , 0xcc , 0xcc , 0x3d , 0x04 , 0xcd , 0xcc , 0xcc , 0x3d};
//    printf("Ccccccc\n");
    memcpy(packet, ip_hdr_effects_data, sizeof ip_hdr_effects_data);
    memcpy(packet + sizeof ip_hdr_effects_data, udp_hdr_effects_data, sizeof udp_hdr_effects_data);
    memcpy(packet + sizeof ip_hdr_effects_data + sizeof udp_hdr_effects_data, effect, sizeof effect);
//    printf("Ddddddd\n");

    uint16_t ip_csm = inet_csum(packet, 20);
    packet[10] = ip_csm & 0x00FF ;
    packet[11] = (ip_csm >> 8) & 0x00FF ;
    
    uint16_t udp_csm=0;
    packet[26] = udp_csm & 0x00FF ;
    packet[27] = (udp_csm >> 8) & 0x00FF ;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Not enough arguments\n");
    return 0;
  }

  /* PCAP vars */
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *ppcap;

  uint8_t const header_size = sizeof u8aRadiotapHeader + sizeof mac_header + sizeof ip_hdr_song_data + sizeof udp_hdr_song_data;  
  uint8_t buf[header_size + 3*num_samples+1 + 4 /*FCS*/];

  memcpy(buf, u8aRadiotapHeader, sizeof u8aRadiotapHeader);
  memcpy(buf + sizeof u8aRadiotapHeader, mac_header, sizeof mac_header);

  /**
   * Finally, we have the packet and are ready to inject it.
   * First, we open the interface we want to inject on using pcap.
   */
  ppcap = pcap_open_live(argv[1], 800, 1, 20, errbuf);

  if (ppcap == NULL) {
    printf("Could not open interface wlan1 for packet injection: %s", errbuf);
    return 2;
  }

  auto time = chrono::steady_clock::now();
  for(uint8_t i=0; ; ++i){
    if (i%4 == 0) {
      this_thread::sleep_for(chrono::milliseconds(3));
//      fill_effect_packet(buf + sizeof u8aRadiotapHeader + sizeof mac_header);
      memcpy(buf + sizeof u8aRadiotapHeader + sizeof mac_header, eff_pkt, sizeof eff_pkt);
      if(pcap_sendpacket(ppcap, buf, sizeof u8aRadiotapHeader + sizeof mac_header +  sizeof eff_pkt + 4) != 0)
        pcap_perror(ppcap, "Failed to inject effect packet");
      else
        printf("Injected effect packet\n");

      if (i%4 == 0) {
        this_thread::sleep_for(chrono::milliseconds(1));
        int const sz = fill_beacon(buf + sizeof u8aRadiotapHeader);
        if(pcap_sendpacket(ppcap, buf, sz + sizeof u8aRadiotapHeader + 4) != 0)
          pcap_perror(ppcap, "Failed to inject beacon");
        else
          printf("Injected beacon\n");
        memcpy(buf + sizeof u8aRadiotapHeader, mac_header, sizeof mac_header);
      }
    }

    fill_song_packet(buf + sizeof u8aRadiotapHeader + sizeof mac_header);
    if(pcap_sendpacket(ppcap, buf, sizeof buf) != 0)
      pcap_perror(ppcap, "Failed to inject song packet");
    else
      printf("Injected song packet\n");

    time += chrono::milliseconds(16);
    while (chrono::steady_clock::now() < time);
  }
  pcap_close(ppcap);
  return 0;
}