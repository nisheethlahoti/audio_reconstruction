#include <iostream>
#include "receive.h"

using namespace std;

void packet_result_t::log() {
	switch (type) {
		case packet_result_type::invalid_size:
			cout << "Size mismatch: Received " << dec << num1 << ", expected " << packet_size << endl;
			break;
		case packet_result_type::invalid_magic_number:
			cout << "Magic Number does not match: Received " << hex << num1 << endl;
			break;
		case packet_result_type::invalid_crc:
			cout << "CRC mismatch: Received " << hex << num1 << ", expected " << num2 << endl;
			break;
		case packet_result_type::invalid_uid:
			cout << "UID does not match: Received " << hex << num1 << endl;
			break;
		case packet_result_type::older_packet:
			cout << "Received packet " << dec << num1 << " again after " << num2 << ". Dropping\n";
			break;
		case packet_result_type::full_buffer:
			cout << "Buffer full. Dropping packet " << dec << num1 << endl;
			break;
		case packet_result_type::success:
			cout << "Writing packet " << dec << num1 << endl;
			break;
		case packet_result_type::reader_waiting:
			cout << "Reader waiting for " << num1 << " iterations.\n";
			break;
	}
}
