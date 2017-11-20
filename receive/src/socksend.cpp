#include <unistd.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "receive.h"

using namespace std;

atomic<int> buf_size(0);
int sock;

void thread_fn(chrono::time_point<chrono::steady_clock> time) {
	while (true) {
		this_thread::sleep_until(time += duration);
		if (buf_size > 0) {
			log(playing_log());
			buf_size--;
		} else {
			log(reader_waiting_log());
		}
	}
}

void write_samples(void const *samples, size_t len) {
	if (write(sock, samples, len * sizeof sample_t()) < 0) {
		perror("writing on stream socket");
		exit(1);
	}
}

void initialize_receiver() {
	sockaddr_un server;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening stream socket");
		exit(1);
	}

	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, "socket");

	if (connect(sock, reinterpret_cast<sockaddr *>(&server), sizeof server)) {
		close(sock);
		perror("connecting stream socket");
		exit(1);
	}

	thread(thread_fn, chrono::steady_clock::now()).detach();
}
