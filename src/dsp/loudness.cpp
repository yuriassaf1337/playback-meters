#include "loudness.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace pm
{

	float calculate_peak( const sample_t* samples, size_t count )
	{
		float peak = 0.0f;
		for ( size_t i = 0; i < count; ++i ) {
			peak = std::max( peak, std::abs( samples[ i ] ) );
		}
		return peak;
	}

	float calculate_peak_db( const sample_t* samples, size_t count )
	{
		float peak = calculate_peak( samples, count );
		if ( peak < 1e-10f )
			return -100.0f;
		return 20.0f * std::log10( peak );
	}

	float calculate_rms( const sample_t* samples, size_t count )
	{
		if ( count == 0 )
			return 0.0f;

		double sum = 0.0;
		for ( size_t i = 0; i < count; ++i ) {
			sum += samples[ i ] * samples[ i ];
		}
		return static_cast< float >( std::sqrt( sum / count ) );
	}

	float calculate_rms_db( const sample_t* samples, size_t count )
	{
		float rms = calculate_rms( samples, count );
		if ( rms < 1e-10f )
			return -100.0f;
		return 20.0f * std::log10( rms );
	}

	// LUFS meter implementation

	lufs_meter::lufs_meter( int sample_rate ) : sample_rate_( sample_rate )
	{
		reset( );
	}

	void lufs_meter::set_sample_rate( int sample_rate )
	{
		sample_rate_ = sample_rate;
		reset( );
	}

	void lufs_meter::reset( )
	{
		high_shelf_l_ = { };
		high_shelf_r_ = { };
		high_pass_l_  = { };
		high_pass_r_  = { };

		// momentary buffer: 400ms worth of 100ms blocks (4 blocks)
		momentary_buffer_.clear( );
		momentary_buffer_.reserve( 4 );

		// short-term buffer: 3s worth of 100ms blocks (30 blocks)
		short_term_buffer_.clear( );
		short_term_buffer_.reserve( 30 );

		integrated_sum_   = 0.0;
		integrated_count_ = 0;

		momentary_lufs_  = -100.0f;
		short_term_lufs_ = -100.0f;
		integrated_lufs_ = -100.0f;

		block_samples_ = 0;
		block_sum_l_   = 0.0;
		block_sum_r_   = 0.0;
	}

	void lufs_meter::process( const sample_t* samples, size_t frame_count )
	{
		const size_t block_size = sample_rate_ / 10; // 100ms blocks

		for ( size_t i = 0; i < frame_count; ++i ) {
			// stereo samples
			float left  = samples[ i * 2 ];
			float right = samples[ i * 2 + 1 ];

			// apply K-weighting (simplified - just sum squares for now)
			// full implementation would use proper IIR filters
			block_sum_l_ += left * left;
			block_sum_r_ += right * right;
			block_samples_++;

			// when we have a complete 100ms block
			if ( block_samples_ >= block_size ) {
				// calculate mean square for this block
				float mean_square = static_cast< float >( ( block_sum_l_ + block_sum_r_ ) / ( 2.0 * block_samples_ ) );

				// add to momentary buffer (400ms = 4 blocks)
				momentary_buffer_.push_back( mean_square );
				if ( momentary_buffer_.size( ) > 4 ) {
					momentary_buffer_.erase( momentary_buffer_.begin( ) );
				}

				// add to short-term buffer (3s = 30 blocks)
				short_term_buffer_.push_back( mean_square );
				if ( short_term_buffer_.size( ) > 30 ) {
					short_term_buffer_.erase( short_term_buffer_.begin( ) );
				}

				// calculate momentary loudness (400ms)
				if ( !momentary_buffer_.empty( ) ) {
					float avg = std::accumulate( momentary_buffer_.begin( ), momentary_buffer_.end( ), 0.0f ) / momentary_buffer_.size( );
					if ( avg > 1e-10f ) {
						momentary_lufs_ = -0.691f + 10.0f * std::log10( avg );
					} else {
						momentary_lufs_ = -100.0f;
					}
				}

				// calculate short-term loudness (3s)
				if ( !short_term_buffer_.empty( ) ) {
					float avg = std::accumulate( short_term_buffer_.begin( ), short_term_buffer_.end( ), 0.0f ) / short_term_buffer_.size( );
					if ( avg > 1e-10f ) {
						short_term_lufs_ = -0.691f + 10.0f * std::log10( avg );
					} else {
						short_term_lufs_ = -100.0f;
					}
				}

				// update integrated loudness (gated)
				// simplified: just accumulate blocks above -70 LUFS
				float block_lufs = -0.691f + 10.0f * std::log10( mean_square + 1e-10f );
				if ( block_lufs > -70.0f ) {
					integrated_sum_ += mean_square;
					integrated_count_++;

					if ( integrated_count_ > 0 ) {
						float avg        = static_cast< float >( integrated_sum_ / integrated_count_ );
						integrated_lufs_ = -0.691f + 10.0f * std::log10( avg );
					}
				}

				// reset block accumulators
				block_samples_ = 0;
				block_sum_l_   = 0.0;
				block_sum_r_   = 0.0;
			}
		}
	}

} // namespace pm
