#pragma once
#include "../common/types.h"
#include <vector>

namespace pm
{

	// peak measurement
	float calculate_peak( const sample_t* samples, size_t count );
	float calculate_peak_db( const sample_t* samples, size_t count );

	// RMS measurement
	float calculate_rms( const sample_t* samples, size_t count );
	float calculate_rms_db( const sample_t* samples, size_t count );

	// LUFS (simplified ITU-R BS.1770 implementation)
	class lufs_meter
	{
	public:
		explicit lufs_meter( int sample_rate = k_default_sample_rate );

		// process stereo samples (interleaved L/R)
		void process( const sample_t* samples, size_t frame_count );

		// reset the meter
		void reset( );

		// get current values
		float get_momentary( ) const
		{
			return momentary_lufs_;
		} // 400ms window
		float get_short_term( ) const
		{
			return short_term_lufs_;
		} // 3s window
		float get_integrated( ) const
		{
			return integrated_lufs_;
		} // full program

		void set_sample_rate( int sample_rate );

	private:
		int sample_rate_;

		// K-weighting filter state
		struct filter_state {
			double z1 = 0.0, z2 = 0.0;
		};
		filter_state high_shelf_l_, high_shelf_r_;
		filter_state high_pass_l_, high_pass_r_;

		// loudness accumulators
		std::vector< float > momentary_buffer_;  // 400ms blocks
		std::vector< float > short_term_buffer_; // 3s blocks
		double integrated_sum_   = 0.0;
		size_t integrated_count_ = 0;

		float momentary_lufs_  = -100.0f;
		float short_term_lufs_ = -100.0f;
		float integrated_lufs_ = -100.0f;

		size_t block_samples_ = 0;
		double block_sum_l_   = 0.0;
		double block_sum_r_   = 0.0;

		// K-weighting filter coefficients
		void compute_filter_coefficients( );
		double apply_k_weighting( double sample, filter_state& hs, filter_state& hp, bool is_left );
	};

} // namespace pm
