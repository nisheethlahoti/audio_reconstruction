#include <cstdint>
#include <vector>

struct pcap_hdr_t {
    uint32_t magic_number;   /* magic number */
    uint16_t version_major;  /* major version number */
    uint16_t version_minor;  /* minor version number */
    int32_t  thiszone;       /* GMT to local correction */
    uint32_t sigfigs;        /* accuracy of timestamps */
    uint32_t snaplen;        /* max length of captured packets, in octets */
    uint32_t network;        /* data link type */
};

struct pcap_packet_header_t {
    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */
};

struct pcap_packet_t {
    pcap_packet_header_t header;
    uint8_t *data;
    size_t radiotap_length();

    uint64_t time() {return 1000000ull * header.ts_sec + header.ts_usec;}
    uint8_t* packet_pos() {return data + radiotap_length();}
    size_t packet_len() {return header.incl_len - radiotap_length();}
};

class pcap_file {
public:
    pcap_hdr_t header;
    std::vector<pcap_packet_t> packets;
	pcap_file(char const *filename);
	~pcap_file();
};
