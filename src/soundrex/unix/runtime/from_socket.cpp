#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

#include <soundrex/unix/runtime/lib.hpp>

void soundrex_main(std::span<char *const> args) {
	sockaddr_in self;
	self.sin_family = AF_INET;
	self.sin_addr.s_addr = args.empty() ? htonl(INADDR_ANY) : inet_addr(args[0]);
	self.sin_port = htons(9428);

	int sockfd = wrap_error(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP), "creating socket");
	wrap_error(bind(sockfd, (sockaddr const *)&self, sizeof(self)), "binding socket");

	std::array<sample_t, packet_samples> samples;
	std::clog << args[0] << ": reading batches of " << sizeof(samples) << " bytes" << std::endl;

	while (int bytes = wrap_error(read(sockfd, &samples, sizeof(samples)), "reading socket")) {
		if (bytes != sizeof(samples))
			std::clog << "Received only " << bytes << " bytes. Ignoring." << std::endl;
		else
			write(1, &samples, sizeof(samples));
	}
}
