#pragma once

// FFT processing via KissFFT

#include "../common/types.h"
#include <memory>
#include <vector>

namespace pm
{

	enum class fft_window_type {
		none,
		hann,
		hamming,
		blackman
	};

	enum class fft_scale_type {
		linear,
		logarithmic,
		mel
	};

	class fft_processor
	{
	public:
		explicit fft_processor( size_t fft_size = k_fft_size_4096 );
		~fft_processor( );

		// process samples and compute FFT
		void process( const sample_t* input, size_t sample_count );

		// get results
		float get_magnitude( size_t bin ) const;
		float get_magnitude_db( size_t bin ) const;
		float get_frequency( size_t bin ) const;

		// get all magnitudes
		const std::vector< float >& get_magnitudes( ) const
		{
			return magnitudes_;
		}
		const std::vector< float >& get_magnitudes_db( ) const
		{
			return magnitudes_db_;
		}

		// configuration
		void set_fft_size( size_t size );
		size_t get_fft_size( ) const
		{
			return fft_size_;
		}
		size_t get_bin_count( ) const
		{
			return fft_size_ / 2;
		}

		void set_window_type( fft_window_type type );
		void set_sample_rate( int sample_rate )
		{
			sample_rate_ = sample_rate;
		}
		int get_sample_rate( ) const
		{
			return sample_rate_;
		}

		// smoothing for visualization (0.0 = no smoothing, 1.0 = max smoothing)
		void set_smoothing( float smoothing )
		{
			smoothing_ = smoothing;
		}

	private:
		struct impl;
		std::unique_ptr< impl > impl_;

		size_t fft_size_;
		int sample_rate_             = k_default_sample_rate;
		fft_window_type window_type_ = fft_window_type::hann;
		float smoothing_             = 0.8f;

		std::vector< float > window_;
		std::vector< float > magnitudes_;
		std::vector< float > magnitudes_db_;
		std::vector< sample_t > input_buffer_;

		void compute_window( );
	};

} // namespace pm
