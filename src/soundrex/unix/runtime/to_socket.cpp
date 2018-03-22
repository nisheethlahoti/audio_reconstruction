#include <arpa/inet.h>
#include <soundrex/unix/runtime/lib.h>
#include <sys/socket.h>
#include <iostream>

void soundrex_main(slice_t<char *>) {
	sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr("127.0.0.1");
	dest.sin_port = htons(9428);

	int sockfd = wrap_error(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP), "Creating socket");
	std::array<sample_t, packet_samples> samples;

	while (std::cin.read(reinterpret_cast<char *>(&samples), sizeof(samples)))
		sendto(sockfd, &samples, sizeof(samples), 0, (sockaddr const *)&dest, sizeof(dest));
	sendto(sockfd, &samples, 0, 0, (sockaddr const *)&dest, sizeof(dest));
}
