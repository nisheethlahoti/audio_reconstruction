#ifndef SOUNDREX_RECEIVE_H
#define SOUNDREX_RECEIVE_H

#include <constants.h>

struct raw_packet_t {
	uint8_t const *data;
	ssize_t size;
};

#endif /* SOUNDREX_RECEIVE_H */
