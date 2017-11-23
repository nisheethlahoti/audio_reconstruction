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

#include "../src/receive.h"
#include "wav_direct_write.h"

using namespace std;

static sample_t samples[1024];
static char const sockname[] = "/tmp/socket";

void play_samples(void const *samples, size_t len);
void initialize(int num_ch, int byte_dp, uint32_t sample_rate, size_t batch_sz);

int main() {
	initialize(num_channels, byte_depth, samples_per_s, packet_samples);
	wav_header_t header(num_channels, samples_per_s, byte_depth);
	wav_writer_t<sample_t> outp("outp.wav", header);

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
	while ((rval = read(msgsock, samples, packet_samples * sizeof samples[0])) >
	       0) {
		int const num_samples = rval / sizeof samples[0];
		if (num_samples * sizeof samples[0] != rval)
			break;
		play_samples(samples, num_samples);
		for (int i = 0; i < num_samples; ++i)
			outp.add_sample(samples[i]);
	}

	close(msgsock);
	close(sock);
	unlink(sockname);

	if (rval < 0) {
		perror("reading stream message");
		return 1;
	} else if (rval % sizeof samples[0]) {
		cerr << rval << " not divisible by " << sizeof samples[0] << endl;
		return 2;
	} else {
		return 0;
	}
}
