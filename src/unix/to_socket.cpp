#include <arpa/inet.h>
#include <netinet/in.h>
#include <soundrex/constants.h>
#include <sys/socket.h>
#include <unix/soundrex/common.h>
#include <iostream>

void soundrex_main(slice_t<char *>) {
	sockaddr_in dest;
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr("127.0.0.1");
	dest.sin_port = htons(9428);

	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0)
		throw std::runtime_error("Can't create socket.");

	std::string tname;
	std::getline(std::cin, tname);
	std::cerr << "Setting terminal to " << tname << '\n';

	std::array<sample_t, packet_samples> samples;
	while (std::cin.read(reinterpret_cast<char *>(&samples), sizeof(samples)))
		sendto(sockfd, &samples, sizeof(samples), 0, (sockaddr const *)&dest, sizeof(dest));
}
