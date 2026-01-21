#include "audio_engine.h"
#include <algorithm>
#include <cmath>
#include <combaseapi.h>

#undef max

namespace pm
{

	audio_engine::audio_engine( ) = default;

	audio_engine::~audio_engine( )
	{
		shutdown( );
	}

	bool audio_engine::initialize( )
	{
		if ( initialized_ )
			return true;

		// initialize COM for this thread (audio operations)
		HRESULT hr = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
		if ( FAILED( hr ) && hr != RPC_E_CHANGED_MODE ) {
			return false;
		}

		if ( !device_enumerator_.initialize( ) ) {
			return false;
		}

		// set up audio callback for level tracking
		capture_.set_callback( [ this ]( const sample_t* samples, size_t frame_count, int channels ) {
			float max_left  = 0.0f;
			float max_right = 0.0f;

			for ( size_t i = 0; i < frame_count; ++i ) {
				if ( channels >= 1 ) {
					float l  = std::abs( samples[ i * channels ] );
					max_left = std::max( max_left, l );
				}
				if ( channels >= 2 ) {
					float r   = std::abs( samples[ i * channels + 1 ] );
					max_right = std::max( max_right, r );
				}
			}

			// simple peak hold with decay
			peak_left_  = std::max( peak_left_ * 0.95f, max_left );
			peak_right_ = std::max( peak_right_ * 0.95f, max_right );
		} );

		initialized_ = true;

		// auto-start capturing system output (loopback)
		start_capture( L"", true );

		return true;
	}

	void audio_engine::shutdown( )
	{
		if ( !initialized_ )
			return;

		stop_capture( );
		device_enumerator_.shutdown( );
		initialized_ = false;
	}

	bool audio_engine::start_capture( const std::wstring& device_id, bool loopback )
	{
		return capture_.start( device_id, loopback );
	}

	void audio_engine::stop_capture( )
	{
		capture_.stop( );
	}

	void audio_engine::get_peak_levels( float& left, float& right )
	{
		left  = peak_left_;
		right = peak_right_;
	}

} // namespace pm
