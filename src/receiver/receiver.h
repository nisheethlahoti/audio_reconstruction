#include <atomic>

#include <constants.h>

struct batch_t {
	uint32_t num;
	std::array<sample_t, packet_samples> samples;
	std::array<sample_t, trailing_samples> trailing;
};

// None of its public functions can be called from more than one thread.
// However, each of them can be called from its own separate thread.
class receiver_t {
	static constexpr size_t max_buf_size = 3;
	std::atomic<bool> correction_on = true;
	std::array<batch_t, max_buf_size + 1> batches;

	typedef decltype(batches)::iterator b_itr;
	typedef decltype(batches)::const_iterator b_const_itr;

	uint32_t latest_packet_number = 0;
	std::atomic<b_itr> b_begin = batches.begin(), b_end = batches.begin() + 1;

	bool write_packet(uint8_t const *packet, uint32_t pnum);
	void mergewrite_samples(b_const_itr first, b_const_itr second);

   public:
	bool receive_callback(slice_t packet);  // Returns whether packet is SoundRex packet
	void play_next();
	bool toggle_corrections();  // Returns whether corrections are now on.
};
