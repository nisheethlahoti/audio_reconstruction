#include <net/if.h>
#include <pcap.h>
#include <vector>

#include <constants.h>

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

	slice_t get_packet();
	void inject(slice_t);

	void setfilter(char const *);
	unsigned int getrecv();
};

std::vector<capture_t> open_captures(int num, char **names);
