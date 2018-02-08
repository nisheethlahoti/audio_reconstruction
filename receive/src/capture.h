#include <pcap.h>
#include "receive.h"

class capture_t {
	pcap_t *pcap;
	int fd_;

   public:
	capture_t(char const *);
	capture_t(capture_t &&);
	int fd() const;
	raw_packet_t get_packet();
	~capture_t();
};
