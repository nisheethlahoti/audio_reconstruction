#include <soundrex/constants.h>
#include <atomic>

enum class reconstruct_t { white_noise, smooth_merge, silence, sharp_merge, reconstruct };

// None of its public functions can be called from more than one thread.
// However, each of them can be called from its own separate thread.
class processor_t {
	static constexpr size_t max_buf_size = 3;
	std::array<packet_t, max_buf_size + 1> batches;

	typedef decltype(batches)::iterator b_itr;

	uint32_t latest_packet_number = 0;
	std::atomic<b_itr> b_begin = batches.begin(), b_end = batches.begin() + 1;

   public:
	std::atomic<reconstruct_t> reconstruct = reconstruct_t::reconstruct;
	void process(uint8_t const* packet);
	void play_next();
};
