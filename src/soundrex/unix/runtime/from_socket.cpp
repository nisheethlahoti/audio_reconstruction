#include <arpa/inet.h>
#include <soundrex/unix/runtime/lib.h>
#include <unistd.h>
#include <iostream>

void soundrex_main(slice_t<char *> args) {
	sockaddr_in self;
	self.sin_family = AF_INET;
	self.sin_addr.s_addr = args.empty() ? htonl(INADDR_ANY) : inet_addr(args[0]);
	self.sin_port = htons(9428);

	int sockfd = wrap_error(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP), "creating socket");
	wrap_error(bind(sockfd, (sockaddr const *)&self, sizeof(self)), "binding socket");

	std::array<sample_t, packet_samples> samples;
	std::cerr << args[0] << ": reading batches of " << sizeof(samples) << " bytes\n";

	while (int bytes = wrap_error(read(sockfd, &samples, sizeof(samples)), "reading socket")) {
		if (bytes != sizeof(samples))
			std::cerr << "Received only " << bytes << " bytes. Ignoring.\n";
		else
			std::cout.write(reinterpret_cast<char const *>(&samples), sizeof(samples));
	}
}
