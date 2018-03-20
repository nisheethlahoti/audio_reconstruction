// WIFI CRC POLYNOMIAL IS 1 00000100 11000001 00011101 10110111

#include <pcap.h>
#include <soundrex/constants.h>
#include <unix/soundrex/capture.h>
#include <unix/soundrex/common.h>
#include <algorithm>
#include <iostream>

constexpr std::array<uint8_t, 24> const radiotap_hdr = {{
    /*0*/ 0x00, 0x00,  // radiotap version (ignore this)
    /*2*/ 0x18, 0x00,  // number of bytes in our header

    /**
     *   0x00 0x01: timestamp
     *   0x00 0x02: flags
     *   0x00 0x03: rate
     *   0x00 0x04: channel
     *   0x80 0x00: tx flags (seems silly to have this AND flags, but oh well)
     */
    /*4*/ 0x0f, 0x80, 0x00, 0x00,  // included bitmaps

    /*8*/ 0x00, 0x00, 0x00, 0x00,
    /*12*/ 0x00, 0x00, 0x00, 0x00,  // timestamp
    /*16*/ 0x10,                    // yes, the card should add FCS at the end of the mac frame
    /*17*/ 0x18,                    // rate
    /*18*/ 0x6c, 0x09, 0xA0, 0x00,  // channel

    // The second set of flags, specifically related to transmissions.
    /*22*/ 0x08, 0x00  // Don't wait for ACK
}};

constexpr std::array<uint8_t, 32> mac_header = {{
    /*0*/ 0x08,   0x00,                                    // Frame Control for data frame
    /*2*/ 0x01,   0x01,                                    // Duration
    /*4*/ pin[0], pin[1], pin[2], pin[3], pin[4], pin[5],  // Destination address
    /*10*/ 0x6e,  0x40,   0x08,   0x49,   0x01,   0x64,    // Source address
    /*16*/ 0x6e,  0x40,   0x08,   0x49,   0x01,   0x64,    // BSSID
    /*22*/ 0x00,  0x00,                                    // Seq-ctl
    /*24*/ 0xaa,  0xaa,                                    // IPLLC SNAP header
    /*26*/ 0x03,  0x00,   0x00,   0x00,   0x08,   0x00     // SNAP field
}};

int main(int argc, char **argv) try {
	if (argc < 4)
		throw std::invalid_argument(std::string(argv[0]) + " <2*rate> <redundancy> <ifaces...>");

	std::array<uint8_t, radiotap_hdr.size() + mac_header.size() + sizeof(packet_t) + 4> buf;
	std::copy(radiotap_hdr.begin(), radiotap_hdr.end(), buf.begin());
	std::copy(mac_header.begin(), mac_header.end(), buf.begin() + radiotap_hdr.size());
	uint8_t *const packet_loc = buf.data() + radiotap_hdr.size() + mac_header.size();

	buf[17] = atoi(argv[1]);
	int redundancy = atoi(argv[2]);
	if (redundancy <= 0) {
		std::cerr << "Redundancy should be positive, not " << redundancy << '\n';
		return 1;
	}

	std::vector<capture_t> captures = open_captures(argc - 3, argv + 3);

	set_realtime();
	while (std::cin.read(reinterpret_cast<char *>(packet_loc), sizeof(packet_t)))
		for (int i = 0; i < redundancy; ++i)
			for (auto &cap : captures)
				cap.inject(slice_t{buf.data(), buf.size()});
} catch (std::exception const &expt) {
	std::cerr << "Error: " << expt.what() << '\n';
	return 1;
}
