// WASAPI audio capture implementation

#include "audio_capture.h"

#define WIN32_LEAN_AND_MEAN
#include <audioclient.h>
#include <avrt.h>
#include <mmdeviceapi.h>
#include <windows.h>
#include <wrl/client.h>

#pragma comment( lib, "avrt.lib" )

using Microsoft::WRL::ComPtr;

namespace pm
{

	// ring buffer for captured samples (10 seconds at 48kHz stereo)
	static constexpr size_t k_ring_buffer_size = 48000 * 2 * 10;

	struct audio_capture::impl {
		ComPtr< IMMDevice > device;
		ComPtr< IAudioClient > audio_client;
		ComPtr< IAudioCaptureClient > capture_client;
		HANDLE event_handle = nullptr;

		ring_buffer< sample_t, k_ring_buffer_size > buffer;
		bool is_loopback = false;
	};

	audio_capture::audio_capture( ) : impl_( std::make_unique< impl >( ) ) { }

	audio_capture::~audio_capture( )
	{
		stop( );
	}

	bool audio_capture::start( const std::wstring& device_id, bool is_loopback )
	{
		if ( capturing_ ) {
			stop( );
		}

		impl_->is_loopback = is_loopback;

		// get device
		ComPtr< IMMDeviceEnumerator > enumerator;
		HRESULT hr = CoCreateInstance( __uuidof( MMDeviceEnumerator ), nullptr, CLSCTX_ALL, __uuidof( IMMDeviceEnumerator ),
		                               reinterpret_cast< void** >( enumerator.GetAddressOf( ) ) );
		if ( FAILED( hr ) )
			return false;

		if ( device_id.empty( ) ) {
			// default device
			hr = enumerator->GetDefaultAudioEndpoint( is_loopback ? eRender : eCapture, eConsole, &impl_->device );
		} else {
			hr = enumerator->GetDevice( device_id.c_str( ), &impl_->device );
		}
		if ( FAILED( hr ) )
			return false;

		// create audio client
		hr = impl_->device->Activate( __uuidof( IAudioClient ), CLSCTX_ALL, nullptr,
		                              reinterpret_cast< void** >( impl_->audio_client.GetAddressOf( ) ) );
		if ( FAILED( hr ) )
			return false;

		// get mix format
		WAVEFORMATEX* mix_format = nullptr;
		hr                       = impl_->audio_client->GetMixFormat( &mix_format );
		if ( FAILED( hr ) )
			return false;

		sample_rate_ = mix_format->nSamplesPerSec;
		channels_    = mix_format->nChannels;

		// initialize audio client
		DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		if ( is_loopback ) {
			stream_flags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
		}

		REFERENCE_TIME buffer_duration = 10000000; // 1 second
		hr = impl_->audio_client->Initialize( AUDCLNT_SHAREMODE_SHARED, stream_flags, buffer_duration, 0, mix_format, nullptr );
		CoTaskMemFree( mix_format );
		if ( FAILED( hr ) )
			return false;

		// create event handle
		impl_->event_handle = CreateEventW( nullptr, FALSE, FALSE, nullptr );
		if ( !impl_->event_handle )
			return false;

		hr = impl_->audio_client->SetEventHandle( impl_->event_handle );
		if ( FAILED( hr ) )
			return false;

		// get capture client
		hr = impl_->audio_client->GetService( __uuidof( IAudioCaptureClient ), reinterpret_cast< void** >( impl_->capture_client.GetAddressOf( ) ) );
		if ( FAILED( hr ) )
			return false;

		// start capture
		hr = impl_->audio_client->Start( );
		if ( FAILED( hr ) )
			return false;

		capturing_      = true;
		capture_thread_ = std::thread( &audio_capture::capture_loop, this );

		return true;
	}

	void audio_capture::stop( )
	{
		if ( !capturing_ )
			return;

		capturing_ = false;

		if ( impl_->event_handle ) {
			SetEvent( impl_->event_handle ); // wake up the thread
		}

		if ( capture_thread_.joinable( ) ) {
			capture_thread_.join( );
		}

		if ( impl_->audio_client ) {
			impl_->audio_client->Stop( );
		}

		if ( impl_->event_handle ) {
			CloseHandle( impl_->event_handle );
			impl_->event_handle = nullptr;
		}

		impl_->capture_client.Reset( );
		impl_->audio_client.Reset( );
		impl_->device.Reset( );
	}

	void audio_capture::capture_loop( )
	{
		// boost thread priority for audio
		DWORD task_index   = 0;
		HANDLE task_handle = AvSetMmThreadCharacteristicsW( L"Pro Audio", &task_index );

		while ( capturing_ ) {
			DWORD wait_result = WaitForSingleObject( impl_->event_handle, 100 );

			if ( !capturing_ )
				break;
			if ( wait_result != WAIT_OBJECT_0 )
				continue;

			UINT32 packet_length = 0;
			impl_->capture_client->GetNextPacketSize( &packet_length );

			while ( packet_length > 0 ) {
				BYTE* data        = nullptr;
				UINT32 num_frames = 0;
				DWORD flags       = 0;

				HRESULT hr = impl_->capture_client->GetBuffer( &data, &num_frames, &flags, nullptr, nullptr );
				if ( FAILED( hr ) )
					break;

				if ( !( flags & AUDCLNT_BUFFERFLAGS_SILENT ) ) {
					const sample_t* samples = reinterpret_cast< const sample_t* >( data );
					size_t sample_count     = num_frames * channels_;

					// push to ring buffer
					impl_->buffer.push( samples, sample_count );

					// call callback if set
					if ( callback_ ) {
						callback_( samples, num_frames, channels_ );
					}
				}

				impl_->capture_client->ReleaseBuffer( num_frames );
				impl_->capture_client->GetNextPacketSize( &packet_length );
			}
		}

		if ( task_handle ) {
			AvRevertMmThreadCharacteristics( task_handle );
		}
	}

	size_t audio_capture::get_samples( sample_t* dest, size_t max_samples )
	{
		return impl_->buffer.pop( dest, max_samples );
	}

	size_t audio_capture::samples_available( ) const
	{
		return impl_->buffer.available( );
	}

} // namespace pm
