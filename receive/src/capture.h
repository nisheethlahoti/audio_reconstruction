#include <pcap.h>
#include "receive.h"

class capture_t {
	pcap_t *pcap;
	int fd_;

   public:
	capture_t(char const *);
	int fd() const;
	raw_packet_t get_packet() const;
	~capture_t();
};
