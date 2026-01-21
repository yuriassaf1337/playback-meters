#pragma once

#include "../common/types.h"
#include "audio_capture.h"
#include "device_enumerator.h"
#include <memory>

namespace pm
{

	class audio_engine
	{
	public:
		audio_engine( );
		~audio_engine( );

		bool initialize( );
		void shutdown( );

		// device management
		device_enumerator& get_device_enumerator( )
		{
			return device_enumerator_;
		}
		const device_enumerator& get_device_enumerator( ) const
		{
			return device_enumerator_;
		}

		// start/stop capture
		bool start_capture( const std::wstring& device_id = L"", bool loopback = true );
		void stop_capture( );
		bool is_capturing( ) const
		{
			return capture_.is_capturing( );
		}

		// audio access
		audio_capture& get_capture( )
		{
			return capture_;
		}
		const audio_capture& get_capture( ) const
		{
			return capture_;
		}

		// get current audio levels (peak)
		void get_peak_levels( float& left, float& right );

	private:
		device_enumerator device_enumerator_;
		audio_capture capture_;
		bool initialized_ = false;

		// level tracking
		float peak_left_  = 0.0f;
		float peak_right_ = 0.0f;
	};

} // namespace pm
