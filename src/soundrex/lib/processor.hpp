#include <atomic>

#include <soundrex/constants.hpp>

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
	void mergewrite_samples(b_const_itr first, b_const_itr second) const;

   public:
	void process(uint8_t const *packet);
	void play_next();
	bool toggle_corrections();  // Returns whether corrections are now on.
};
