#pragma once

#include "../common/types.h"
#include <locale>
#include <string>
#include <vector>

namespace pm
{

	struct audio_device_info {
		std::wstring id;   // ID for WASAPI
		std::wstring name; // friendly name
		device_type type;  // input or output
		bool is_default;   // is system default
	};

	class device_enumerator
	{
	public:
		device_enumerator( );
		~device_enumerator( );

		bool initialize( );
		void shutdown( );

		// get available devices
		std::vector< audio_device_info > get_input_devices( ) const;
		std::vector< audio_device_info > get_output_devices( ) const;
		std::vector< audio_device_info > get_all_devices( ) const;

		// get default device
		audio_device_info get_default_input_device( ) const;
		audio_device_info get_default_output_device( ) const;

		// refresh device list (call after device changes)
		void refresh( );

	private:
		struct impl;
		std::unique_ptr< impl > impl_;
	};

} // namespace pm
