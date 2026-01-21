#include "spectrogram.h"
#include <algorithm>
#include <cmath>

namespace pm
{

	spectrogram::spectrogram( ) : meter_panel( "Spectrogram" ), fft_( k_fft_size_2048 )
	{
		mono_buffer_.resize( k_fft_size_2048 );

		// initialize history
		history_.resize( k_history_width );
		for ( auto& col : history_ ) {
			col.resize( k_display_rows, -100.0f );
		}
	}

	void spectrogram::set_fft_size( size_t size )
	{
		fft_.set_fft_size( size );
		mono_buffer_.resize( size );
	}

	// convert display row (0 to k_display_rows-1) to frequency using logarithmic scale
	static float row_to_freq( int row, int total_rows )
	{
		float t = static_cast< float >( row ) / ( total_rows - 1 );
		return k_min_freq * std::pow( k_max_freq / k_min_freq, t );
	}

	// convert frequency to FFT bin index
	static size_t freq_to_bin( float freq, size_t bin_count, float sample_rate )
	{
		float bin_width = sample_rate / ( bin_count * 2.0f );
		size_t bin      = static_cast< size_t >( freq / bin_width );
		return std::min( bin, bin_count - 1 );
	}

	// get averaged magnitude for a frequency range
	static float get_band_db( const fft_processor& fft, float freq_start, float freq_end )
	{
		size_t bin_count  = fft.get_bin_count( );
		float sample_rate = static_cast< float >( fft.get_sample_rate( ) );

		size_t bin_start = freq_to_bin( freq_start, bin_count, sample_rate );
		size_t bin_end   = freq_to_bin( freq_end, bin_count, sample_rate );

		if ( bin_start >= bin_end )
			bin_end = bin_start + 1;
		if ( bin_end > bin_count )
			bin_end = bin_count;

		float sum = 0.0f;
		for ( size_t i = bin_start; i < bin_end; ++i ) {
			float mag = fft.get_magnitude( i );
			sum += mag * mag;
		}

		float avg_power = sum / ( bin_end - bin_start );
		float avg_mag   = std::sqrt( avg_power );

		return ( avg_mag > 1e-10f ) ? 20.0f * std::log10( avg_mag ) : -100.0f;
	}

	void spectrogram::update( const sample_t* samples, size_t frame_count, int channels )
	{
		// convert to mono
		size_t copy_count = std::min( frame_count, mono_buffer_.size( ) );
		for ( size_t i = 0; i < copy_count; ++i ) {
			if ( channels >= 2 ) {
				mono_buffer_[ i ] = ( samples[ i * channels ] + samples[ i * channels + 1 ] ) * 0.5f;
			} else if ( channels >= 1 ) {
				mono_buffer_[ i ] = samples[ i * channels ];
			}
		}

		fft_.process( mono_buffer_.data( ), copy_count );

		update_counter_++;
		if ( update_counter_ >= updates_per_column_ ) {
			update_counter_ = 0;

			// store FFT magnitudes in history (with proper frequency mapping)
			auto& col = history_[ write_pos_ ];

			for ( int row = 0; row < k_display_rows; ++row ) {
				float freq      = row_to_freq( row, k_display_rows );
				float freq_next = row_to_freq( row + 1, k_display_rows );
				col[ row ]      = get_band_db( fft_, freq, freq_next );
			}

			write_pos_ = ( write_pos_ + 1 ) % k_history_width;
		}
	}

	void spectrogram::render( )
	{
		ImVec2 canvas_pos  = ImGui::GetCursorScreenPos( );
		ImVec2 canvas_size = ImGui::GetContentRegionAvail( );

		if ( canvas_size.x < 50 || canvas_size.y < 50 )
			return;

		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		// background
		draw_list->AddRectFilled( canvas_pos, ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ), IM_COL32( 5, 5, 10, 255 ) );

		float col_width  = canvas_size.x / k_history_width;
		float row_height = canvas_size.y / k_display_rows;

		for ( size_t t = 0; t < k_history_width; ++t ) {
			size_t idx      = ( write_pos_ + t ) % k_history_width;
			const auto& col = history_[ idx ];

			float x = canvas_pos.x + t * col_width;

			for ( int row = 0; row < k_display_rows; ++row ) {
				float db    = col[ row ];
				ImU32 color = db_to_color( db );

				// low frequencies at bottom
				float y = canvas_pos.y + canvas_size.y - ( row + 1 ) * row_height;

				draw_list->AddRectFilled( ImVec2( x, y ), ImVec2( x + col_width + 1, y + row_height + 1 ), color );
			}
		}

		// draw frequency labels on right side
		float freqs[]        = { 100.0f, 1000.0f, 10000.0f };
		const char* labels[] = { "100Hz", "1kHz", "10kHz" };

		for ( int i = 0; i < 3; ++i ) {
			float t = std::log( freqs[ i ] / k_min_freq ) / std::log( k_max_freq / k_min_freq );
			float y = canvas_pos.y + canvas_size.y - t * canvas_size.y;

			draw_list->AddLine( ImVec2( canvas_pos.x, y ), ImVec2( canvas_pos.x + 10, y ), IM_COL32( 150, 150, 150, 200 ) );
			draw_list->AddText( ImVec2( canvas_pos.x + 12, y - 6 ), IM_COL32( 150, 150, 150, 200 ), labels[ i ] );
		}

		ImGui::Dummy( canvas_size );
	}

	ImU32 spectrogram::db_to_color( float db )
	{
		float t = ( db - min_db_ ) / ( max_db_ - min_db_ );
		t       = std::max( 0.0f, std::min( 1.0f, t ) );

		// Black -> Deep Blue -> Cyan -> Green -> Yellow -> Orange -> Red -> White
		if ( t < 0.05f ) {
			float lt = t / 0.05f;
			return lerp_color( IM_COL32( 0, 0, 0, 255 ), IM_COL32( 20, 10, 40, 255 ), lt );
		} else if ( t < 0.15f ) {
			float lt = ( t - 0.05f ) / 0.1f;
			return lerp_color( IM_COL32( 20, 10, 40, 255 ), IM_COL32( 30, 50, 150, 255 ), lt );
		} else if ( t < 0.30f ) {
			float lt = ( t - 0.15f ) / 0.15f;
			return lerp_color( IM_COL32( 30, 50, 150, 255 ), IM_COL32( 40, 150, 180, 255 ), lt );
		} else if ( t < 0.45f ) {
			float lt = ( t - 0.30f ) / 0.15f;
			return lerp_color( IM_COL32( 40, 150, 180, 255 ), IM_COL32( 80, 200, 80, 255 ), lt );
		} else if ( t < 0.60f ) {
			float lt = ( t - 0.45f ) / 0.15f;
			return lerp_color( IM_COL32( 80, 200, 80, 255 ), IM_COL32( 200, 220, 50, 255 ), lt );
		} else if ( t < 0.75f ) {
			float lt = ( t - 0.60f ) / 0.15f;
			return lerp_color( IM_COL32( 200, 220, 50, 255 ), IM_COL32( 240, 150, 30, 255 ), lt );
		} else if ( t < 0.90f ) {
			float lt = ( t - 0.75f ) / 0.15f;
			return lerp_color( IM_COL32( 240, 150, 30, 255 ), IM_COL32( 230, 50, 30, 255 ), lt );
		} else {
			float lt = ( t - 0.90f ) / 0.10f;
			return lerp_color( IM_COL32( 230, 50, 30, 255 ), IM_COL32( 255, 255, 255, 255 ), lt );
		}
	}

} // namespace pm
