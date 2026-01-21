#pragma once

// WASAPI audio capture client

#include "../common/types.h"
#include "../dsp/ring_buffer.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace pm
{

	// audio data callback
	using audio_callback_t = std::function< void( const sample_t* samples, size_t frame_count, int channels ) >;

	class audio_capture
	{
	public:
		audio_capture( );
		~audio_capture( );

		// start capturing from device
		// if is_loopback is true, captures "what you hear" from output device
		bool start( const std::wstring& device_id, bool is_loopback = false );
		void stop( );

		bool is_capturing( ) const
		{
			return capturing_.load( );
		}

		// get current audio format
		int get_sample_rate( ) const
		{
			return sample_rate_;
		}

		int get_channels( ) const
		{
			return channels_;
		}

		// set callback for incoming audio data
		void set_callback( audio_callback_t callback )
		{
			callback_ = std::move( callback );
		}

		// direct sample access (thread-safe)
		size_t get_samples( sample_t* dest, size_t max_samples );
		size_t samples_available( ) const;

	private:
		struct impl;
		std::unique_ptr< impl > impl_;

		std::atomic< bool > capturing_{ false };
		std::thread capture_thread_;
		audio_callback_t callback_;

		int sample_rate_ = k_default_sample_rate;
		int channels_    = k_default_channels;

		void capture_loop( );
	};

} // namespace pm
