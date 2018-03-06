#include <pcap.h>
#include <string>
#include "receive.h"

class capture_t {
	pcap_t *pcap;
	std::string name_;
	int fd_;
	unsigned int recv;

   public:
	capture_t(char const *);
	capture_t(capture_t &&);
	~capture_t();

	int fd() const;
	std::string const &name() const;
	raw_packet_t get_packet();

	void addrecv();
	unsigned int getrecv();
};
