#include <sys/select.h>

// Just a C++ style wrapper around the select() syscall
class multiplexer_t {
	fd_set fdset = {}, tmpset;
	int fdmax = -1;

   public:
	void add_fd(int fd);
	void next() noexcept(false);
	bool is_ready(int fd) const;
};
