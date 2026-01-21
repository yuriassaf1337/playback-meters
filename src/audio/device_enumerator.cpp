// audio device enumeration via WASAPI

#include "device_enumerator.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <comdef.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace pm
{

	struct device_enumerator::impl {
		ComPtr< IMMDeviceEnumerator > enumerator;
		std::vector< audio_device_info > input_devices;
		std::vector< audio_device_info > output_devices;
		audio_device_info default_input;
		audio_device_info default_output;

		bool enumerate_devices( EDataFlow flow, std::vector< audio_device_info >& out_devices )
		{
			out_devices.clear( );

			if ( !enumerator ) {
				return false;
			}

			ComPtr< IMMDeviceCollection > collection;
			HRESULT hr = enumerator->EnumAudioEndpoints( flow, DEVICE_STATE_ACTIVE, &collection );
			if ( FAILED( hr ) ) {
				return false;
			}

			UINT count = 0;
			collection->GetCount( &count );

			for ( UINT i = 0; i < count; ++i ) {
				ComPtr< IMMDevice > device;
				hr = collection->Item( i, &device );
				if ( FAILED( hr ) )
					continue;

				audio_device_info info;
				info.type       = ( flow == eCapture ) ? device_type::input : device_type::output;
				info.is_default = false;

				// get device ID
				LPWSTR device_id = nullptr;
				hr               = device->GetId( &device_id );
				if ( SUCCEEDED( hr ) && device_id ) {
					info.id = device_id;
					CoTaskMemFree( device_id );
				}

				// get friendly name
				ComPtr< IPropertyStore > props;
				hr = device->OpenPropertyStore( STGM_READ, &props );
				if ( SUCCEEDED( hr ) ) {
					PROPVARIANT var_name;
					PropVariantInit( &var_name );
					hr = props->GetValue( PKEY_Device_FriendlyName, &var_name );
					if ( SUCCEEDED( hr ) && var_name.vt == VT_LPWSTR ) {
						info.name = var_name.pwszVal;
					}
					PropVariantClear( &var_name );
				}

				out_devices.push_back( std::move( info ) );
			}

			return true;
		}

		bool get_default_device( EDataFlow flow, audio_device_info& out_info )
		{
			if ( !enumerator ) {
				return false;
			}

			ComPtr< IMMDevice > device;
			HRESULT hr = enumerator->GetDefaultAudioEndpoint( flow, eConsole, &device );
			if ( FAILED( hr ) ) {
				return false;
			}

			out_info.type       = ( flow == eCapture ) ? device_type::input : device_type::output;
			out_info.is_default = true;

			// get device ID
			LPWSTR device_id = nullptr;
			hr               = device->GetId( &device_id );
			if ( SUCCEEDED( hr ) && device_id ) {
				out_info.id = device_id;
				CoTaskMemFree( device_id );
			}

			// get friendly name
			ComPtr< IPropertyStore > props;
			hr = device->OpenPropertyStore( STGM_READ, &props );
			if ( SUCCEEDED( hr ) ) {
				PROPVARIANT var_name;
				PropVariantInit( &var_name );
				hr = props->GetValue( PKEY_Device_FriendlyName, &var_name );
				if ( SUCCEEDED( hr ) && var_name.vt == VT_LPWSTR ) {
					out_info.name = var_name.pwszVal;
				}
				PropVariantClear( &var_name );
			}

			return true;
		}
	};

	device_enumerator::device_enumerator( ) : impl_( std::make_unique< impl >( ) ) { }

	device_enumerator::~device_enumerator( )
	{
		shutdown( );
	}

	bool device_enumerator::initialize( )
	{
		HRESULT hr = CoCreateInstance( __uuidof( MMDeviceEnumerator ), nullptr, CLSCTX_ALL, __uuidof( IMMDeviceEnumerator ),
		                               reinterpret_cast< void** >( impl_->enumerator.GetAddressOf( ) ) );

		if ( FAILED( hr ) ) {
			return false;
		}

		refresh( );
		return true;
	}

	void device_enumerator::shutdown( )
	{
		impl_->enumerator.Reset( );
		impl_->input_devices.clear( );
		impl_->output_devices.clear( );
	}

	std::vector< audio_device_info > device_enumerator::get_input_devices( ) const
	{
		return impl_->input_devices;
	}

	std::vector< audio_device_info > device_enumerator::get_output_devices( ) const
	{
		return impl_->output_devices;
	}

	std::vector< audio_device_info > device_enumerator::get_all_devices( ) const
	{
		std::vector< audio_device_info > all;
		all.reserve( impl_->input_devices.size( ) + impl_->output_devices.size( ) );
		all.insert( all.end( ), impl_->input_devices.begin( ), impl_->input_devices.end( ) );
		all.insert( all.end( ), impl_->output_devices.begin( ), impl_->output_devices.end( ) );
		return all;
	}

	audio_device_info device_enumerator::get_default_input_device( ) const
	{
		return impl_->default_input;
	}

	audio_device_info device_enumerator::get_default_output_device( ) const
	{
		return impl_->default_output;
	}

	void device_enumerator::refresh( )
	{
		impl_->enumerate_devices( eCapture, impl_->input_devices );
		impl_->enumerate_devices( eRender, impl_->output_devices );
		impl_->get_default_device( eCapture, impl_->default_input );
		impl_->get_default_device( eRender, impl_->default_output );

		// mark defaults in lists
		for ( auto& dev : impl_->input_devices ) {
			dev.is_default = ( dev.id == impl_->default_input.id );
		}
		for ( auto& dev : impl_->output_devices ) {
			dev.is_default = ( dev.id == impl_->default_output.id );
		}
	}

} // namespace pm
