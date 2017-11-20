#include <unistd.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "../../magic_number.h"
#include "wav_direct_write.h"

using namespace std;

struct sample_t {
	array<uint8_t, 3> val;
	sample_t(uint32_t to_write = 0) {
		uint8_t *init = reinterpret_cast<uint8_t *>(&to_write);
		copy(init, init + 3, val.data());
	}
} samples[1024];

char const sockname[] = "socket";

int main(int argc, char **argv) {
	if (argc < 3) {
		cerr << "Usage: " << argv[0] << " <sample_batch_size> <output_file>\n";
		return 1;
	}

	int const batch_num = atoi(argv[1]);
	wav_header_t header(num_channels, samples_per_s, byte_depth);
	wav_writer_t<sample_t> outp(argv[2], header);

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening stream socket");
		return 1;
	}

	sockaddr_un server;
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, sockname);
	if (bind(sock, reinterpret_cast<sockaddr *>(&server), sizeof server)) {
		perror("binding stream socket");
		return 1;
	}

	listen(sock, 1);
	int msgsock = accept(sock, nullptr, nullptr);
	if (msgsock == -1) {
		perror("accept");
	}

	int rval;
	while ((rval = read(msgsock, samples, batch_num * sizeof samples[0])) > 0) {
		for (int i = 0; i < rval / sizeof samples[0]; ++i)
			outp.add_sample(samples[i]);
	}

	if (rval < 0)
		perror("reading stream message");

	close(msgsock);
	close(sock);
	unlink(sockname);
	return 0;
}
