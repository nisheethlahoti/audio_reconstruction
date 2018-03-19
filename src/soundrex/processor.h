#include <soundrex/constants.h>
#include <atomic>

// None of its public functions can be called from more than one thread.
// However, each of them can be called from its own separate thread.
class processor_t {
	static constexpr size_t max_buf_size = 3;
	std::atomic<bool> correction_on = true;
	std::array<packet_t, max_buf_size + 1> batches;

	typedef decltype(batches)::iterator b_itr;
	typedef decltype(batches)::const_iterator b_const_itr;

	uint32_t latest_packet_number = 0;
	std::atomic<b_itr> b_begin = batches.begin(), b_end = batches.begin() + 1;

	bool write_packet(uint8_t const *packet, uint32_t pnum);
	void mergewrite_samples(b_const_itr first, b_const_itr second) const;

   public:
	void process(slice_t packet);
	void play_next();
	bool toggle_corrections();  // Returns whether corrections are now on.
};
