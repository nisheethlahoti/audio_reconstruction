// WIFI CRC POLYNOMIAL IS 1 00000100 11000001 00011101 10110111

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <fstream>

#include <pcap.h>
#include <unistd.h>
#include "magic_number.h"

using namespace std;
using namespace chrono;

uint16_t const redundancy = 4;
uint32_t packet_num = 1;

uint8_t u8aRadiotapHeader[] = {
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

array<uint8_t, 32> const mac_header = { 
            /*0*/   0x08, 0x00, //Frame Control for data frame 
            /*2*/   0x01, 0x01, //Duration
            /*4*/   0x01, 0x00, 0x5e, 0x1c, 0x04, 0x5e,
            /*10*/  0x6e, 0x40, 0x08, 0x49, 0x01, 0x64, //Source address - overwritten later
            /*16*/  0x6e, 0x40, 0x08, 0x49, 0x01, 0x64, //BSSID - overwritten to the same as the source address
            /*22*/  0x00, 0x00, //Seq-ctl
            
            //addr4 is not present if not WDS(bridge)                               
            //IPLLC SNAP header : next 2 bytes, SNAP field : next 6 bytes
            /*24*/  0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00,
            //Frame body starts here. contains IP packet, UDP packet
            /*32*/  
};

struct wav_chunk {
  std::string name;
  uint32_t size = 0;
  uint8_t *data = nullptr;
  void destroy() {delete[] data;}
};

class wav_reader final: public std::ifstream {
public:
  wav_reader(char const *fname): std::ifstream(fname) {}

  wav_chunk readChunk() {
    char temp[5] = {};
    wav_chunk ret;

    read(temp, 4);
    ret.name = std::string(temp);

    read(temp, 4);
    if (ret.name=="RIFF")
      ret.size = 4;
    else for (int j=0; j<4; ++j)
      ret.size |= (uint8_t)temp[j]<<(uint32_t)(8*j);

    ret.data = new uint8_t[ret.size];
    read((char*)ret.data, ret.size);
    return ret;
  }
};

struct wav_song {
  wav_chunk chunk;

  wav_song(char const *fname) {
    wav_reader reader(fname);
    while ((chunk = reader.readChunk()).name != "data") {
      std::cerr << chunk.name << std::endl;
      chunk.destroy();
    }
    reader.close();
  }

  uint16_t operator[] (int const x) const {
    return (chunk.data[4*x] | (uint16_t)chunk.data[4*x+1]<<8U) ^ 1U<<15U; // Flipping highest bit to convert signed int to unsigned.
  }
} song("filename.wav");


template<typename iterator>
void copy_and_shift(iterator begin, iterator end, uint8_t* &loc) {
  copy(begin, end, loc);
  loc += end-begin;
}

size_t song_pos = 0;
uint8_t* fill_packet(uint8_t *pos){
  for (int i=0; i<4; ++i)
    *pos++ = (packet_num>>(8*i)) & 0xff;

  for (int i=0; i<packet_samples; ++i) {
    uint16_t val = song[song_pos++]; // 16 bit mono
    pos[2*i] = val & 0xff; // little endian
    pos[2*i+1] = (val>>8) & 0xff;
  }
  ++packet_num;

  return 2*packet_samples + pos;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Not enough arguments\n";
    return 0;
  }

  if (argc > 2)
	u8aRadiotapHeader[17] = atoi(argv[2]); // = twice the intended data rate.

  /* PCAP vars */
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *ppcap;

  uint8_t buf[2000];
  uint8_t *packet_loc = buf;

  copy_and_shift(u8aRadiotapHeader, u8aRadiotapHeader + sizeof u8aRadiotapHeader, packet_loc);
  copy_and_shift(mac_header.begin(), mac_header.end(), packet_loc);
  copy_and_shift(magic_number.begin(), magic_number.end(), packet_loc);
  copy_and_shift(uid.begin(), uid.end(), packet_loc);

  ppcap = pcap_open_live(argv[1], 800, 1, 20, errbuf);
  if (ppcap == NULL) {
    cerr << "Could not open interface wlan1 for packet injection: " << errbuf;
    return 2;
  }

  auto time = chrono::steady_clock::now();

  for (int iter = 0; song_pos + packet_samples < song.chunk.size/2; ++iter){
    if (iter % 1000 == 0)
      cout << "Sending next thousand packets" << endl;

    uint8_t* endptr = fill_packet(packet_loc);
    for (int i=0; i<redundancy; ++i) {
      if(pcap_sendpacket(ppcap, buf, endptr - buf + 4) != 0)
        pcap_perror(ppcap, "Failed to inject song packet");
    }

    time += chrono::microseconds(packet_samples * 1000000ULL / samples_per_s);
    while (chrono::steady_clock::now() < time);
  }
  pcap_close(ppcap);
  return 0;
}
