// WIFI CRC POLYNOMIAL IS 1 00000100 11000001 00011101 10110111

#include <pcap.h>
#include <soundrex/unix/lib/capture.h>
#include <soundrex/unix/runtime/lib.h>
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

void soundrex_main(slice_t<char *> args) {
	if (args.size() < 2)
		throw std::domain_error("<2*bitrate> <redundancy> <ifaces...>");

	std::array<uint8_t, radiotap_hdr.size() + mac_header.size() + sizeof(packet_t) + 4> buf;
	uint8_t *const packet_loc = copy_all<uint8_t>(buf.data(), {radiotap_hdr, mac_header});

	char *prate, *predun;
	long const drate = std::strtol(args[0], &prate, 0), redun = std::strtol(args[1], &predun, 0);
	if (drate > 255 || drate <= 0 || redun <= 0 || *prate || *predun)
		throw std::runtime_error("bad data rate or redundancy");

	buf[17] = drate;
	std::vector<capture_t> captures = open_captures(args.subspan(2));

	while (std::cin.read(reinterpret_cast<char *>(packet_loc), sizeof(packet_t)))
		for (int i = 0; i < redun; ++i)
			for (auto &cap : captures)
				cap.inject({buf.begin(), buf.end()});
}
