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

static int sock;
char const sock_path[] = "/tmp/socket-client";

void pause_player() {
	char a = 0;
	if (write(sock, &a, 1) < 0) {
		perror("writing pause packet");
		exit(1);
	}
}

void write_samples(void const *samples, size_t len) {
	if (write(sock, samples, len * sizeof sample_t()) < 0) {
		perror("writing samples on socket");
		exit(1);
	}
}

void stop_player() {
	if (write(sock, nullptr, 0) < 0) {
		perror("writing stop packet");
		exit(1);
	}
	unlink(sock_path);
}

void initialize_player() {
	sockaddr_un server{}, client{};

	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("opening stream socket");
		exit(1);
	}

	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, "/tmp/socket-server");

	client.sun_family = AF_UNIX;
	strcpy(client.sun_path, sock_path);

	if (bind(sock, reinterpret_cast<sockaddr *>(&client), sizeof client)) {
		close(sock);
		perror("binding client socket");
		exit(1);
	}

	if (connect(sock, reinterpret_cast<sockaddr *>(&server), sizeof server)) {
		close(sock);
		perror("connecting dgram socket");
		exit(1);
	}
}
