// FFT processing implementation using KissFFT

#include "fft_processor.h"
#include "kiss_fft.h"
#include <algorithm>
#include <cmath>

namespace pm
{

	struct fft_processor::impl {
		kiss_fft_cfg cfg = nullptr;
		std::vector< kiss_fft_cpx > fft_input;
		std::vector< kiss_fft_cpx > fft_output;

		~impl( )
		{
			if ( cfg ) {
				kiss_fft_free( cfg );
			}
		}

		void resize( size_t fft_size )
		{
			if ( cfg ) {
				kiss_fft_free( cfg );
			}
			cfg = kiss_fft_alloc( static_cast< int >( fft_size ), 0, nullptr, nullptr );
			fft_input.resize( fft_size );
			fft_output.resize( fft_size );
		}
	};

	fft_processor::fft_processor( size_t fft_size ) : impl_( std::make_unique< impl >( ) ), fft_size_( fft_size )
	{
		set_fft_size( fft_size );
	}

	fft_processor::~fft_processor( ) = default;

	void fft_processor::set_fft_size( size_t size )
	{
		fft_size_ = size;
		impl_->resize( size );
		magnitudes_.resize( size / 2 );
		magnitudes_db_.resize( size / 2 );
		input_buffer_.resize( size );
		compute_window( );
	}

	void fft_processor::set_window_type( fft_window_type type )
	{
		window_type_ = type;
		compute_window( );
	}

	void fft_processor::compute_window( )
	{
		window_.resize( fft_size_ );

		const float pi = 3.14159265358979323846f;
		const float n  = static_cast< float >( fft_size_ );

		for ( size_t i = 0; i < fft_size_; ++i ) {
			float x = static_cast< float >( i ) / ( n - 1.0f );

			switch ( window_type_ ) {
			case fft_window_type::none:
				window_[ i ] = 1.0f;
				break;
			case fft_window_type::hann:
				window_[ i ] = 0.5f * ( 1.0f - std::cos( 2.0f * pi * x ) );
				break;
			case fft_window_type::hamming:
				window_[ i ] = 0.54f - 0.46f * std::cos( 2.0f * pi * x );
				break;
			case fft_window_type::blackman:
				window_[ i ] = 0.42f - 0.5f * std::cos( 2.0f * pi * x ) + 0.08f * std::cos( 4.0f * pi * x );
				break;
			}
		}
	}

	void fft_processor::process( const sample_t* input, size_t sample_count )
	{
		// copy input and apply window
		size_t copy_count = std::min( sample_count, fft_size_ );

		// zero-pad if needed
		for ( size_t i = 0; i < fft_size_; ++i ) {
			if ( i < copy_count ) {
				impl_->fft_input[ i ].r = input[ i ] * window_[ i ];
			} else {
				impl_->fft_input[ i ].r = 0.0f;
			}
			impl_->fft_input[ i ].i = 0.0f;
		}

		// FFT
		kiss_fft( impl_->cfg, impl_->fft_input.data( ), impl_->fft_output.data( ) );

		// compute magnitudes
		const float scale  = 2.0f / static_cast< float >( fft_size_ );
		const float min_db = -100.0f;

		for ( size_t i = 0; i < fft_size_ / 2; ++i ) {
			float re  = impl_->fft_output[ i ].r;
			float im  = impl_->fft_output[ i ].i;
			float mag = std::sqrt( re * re + im * im ) * scale;

			// apply smoothing
			float prev_mag   = magnitudes_[ i ];
			mag              = prev_mag * smoothing_ + mag * ( 1.0f - smoothing_ );
			magnitudes_[ i ] = mag;

			// convert to dB
			float db            = ( mag > 1e-10f ) ? 20.0f * std::log10( mag ) : min_db;
			magnitudes_db_[ i ] = std::max( db, min_db );
		}
	}

	float fft_processor::get_magnitude( size_t bin ) const
	{
		if ( bin >= magnitudes_.size( ) )
			return 0.0f;
		return magnitudes_[ bin ];
	}

	float fft_processor::get_magnitude_db( size_t bin ) const
	{
		if ( bin >= magnitudes_db_.size( ) )
			return -100.0f;
		return magnitudes_db_[ bin ];
	}

	float fft_processor::get_frequency( size_t bin ) const
	{
		return static_cast< float >( bin * sample_rate_ ) / static_cast< float >( fft_size_ );
	}

} // namespace pm
