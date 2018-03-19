#include <net/if.h>
#include <pcap.h>
#include <soundrex/constants.h>
#include <vector>

class capture_t {
	pcap_t *pcap;
	int fd_ = -1, recv = 0;
	char name_[IF_NAMESIZE];

   public:
	capture_t(char const *);
	capture_t(capture_t &&);
	~capture_t();

	int fd() const;
	char const *name() const;

	slice_t get_packet() noexcept(false);
	void inject(slice_t) noexcept(false);

	void setfilter(char const *) noexcept(false);
	int getrecv();
};

std::vector<capture_t> open_captures(int num, char **names);
