#include <pthread.h>
#include <sched.h>
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

static array<sample_t, packet_samples> samples;
char *const packet = reinterpret_cast<char *>(samples.data());
static char const sockname[] = "/tmp/socket-server";

void play_samples(void const *samples, size_t len);
void initialize(int num_ch, int byte_dp, uint32_t sample_rate, size_t batch_sz);
void player_pause(int enable);

int main() {
	auto const policy = SCHED_RR;
	sched_param const param{(sched_get_priority_max(policy) + sched_get_priority_min(policy)) / 2};
	pthread_setschedparam(pthread_self(), policy, &param);

	initialize(num_channels, byte_depth, samples_per_s, packet_samples);
	wav_header_t header(num_channels, samples_per_s, byte_depth);
	wav_writer_t<sample_t> outp("outp.wav", header);

	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("opening stream socket");
		return 1;
	}

	sockaddr_un server;
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, sockname);
	if (bind(sock, reinterpret_cast<sockaddr *>(&server), sizeof server)) {
		perror("binding dgram socket");
		return 1;
	}

	bool paused = true;
	int bytes;
	while ((bytes = recv(sock, packet, sizeof(samples), 0)) > 0) {
		if (bytes > 1) {
			int const num_samples = bytes / sizeof(sample_t);
			if (num_samples * sizeof(sample_t) != bytes) {
				cerr << "Received partial sample.\n";
				break;
			}

			if (paused) {
				player_pause(0);
				paused = false;
			}

			play_samples(packet, num_samples);
			for (int i = 0; i < num_samples; ++i)
				outp.add_sample(samples[i]);
		} else {
			player_pause(1);
			paused = true;
		}
	}

	if (bytes < 0) {
		perror("error receiving");
	}

	close(sock);
	unlink(sockname);
	return 0;
}
