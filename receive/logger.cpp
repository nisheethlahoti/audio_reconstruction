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
		case packet_result_type::last_packet:
			cout << "Received packet " << dec << num1 << " again.\n";
			break;
		case packet_result_type::full_buffer:
			cout << "Buffer full. Storing packet " << dec << num1 << " for later.\n";
			break;
		case packet_result_type::hard_throwaway:
			cout << "Dropping packet due to full buffer: " << dec << num1 << endl;
			break;
		case packet_result_type::success:
			cout << "Writing packet " << dec << num1 << endl;
			break;
		case packet_result_type::reader_waiting:
			cout << "Reader waiting for " << dec << num1 << " iterations.\n";
			break;
		case packet_result_type::packet_recovered_and_written:
			cout << "Packet " << dec << num1 << " recovered using majority of " << num2 << " and written.\n";
			break;
		case packet_result_type::packet_recovered_but_dropped:
			cout << "Packet " << dec << num1 << " recovered using majority of " << num2 << " but dropped.\n";
			break;
		case packet_result_type::packet_not_recovered:
			cout << "Could not recover " << dec << num1 << " using majority of " << num2 << endl;
			break;
		case packet_result_type::majority_not_found:
			cout << "Majority not found for packet " << dec << num1 << " among " << num2 << " elements.\n";
			break;
		case packet_result_type::not_enough_packets:
			cout << "Only " << dec << num2 << " packets for " << num1 << ". Not enough to get majority.\n";
			break;
	}
}
