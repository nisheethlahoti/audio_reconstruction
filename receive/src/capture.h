#include <net/if.h>
#include <pcap.h>
#include "receive.h"

class capture_t {
	pcap_t *pcap;
	int fd_;
	unsigned int recv;
	char name_[IF_NAMESIZE];

   public:
	capture_t(char const *);
	capture_t(capture_t &&);
	~capture_t();

	int fd() const;
	char const *name() const;
	raw_packet_t get_packet();

	void addrecv();
	unsigned int getrecv();
};
