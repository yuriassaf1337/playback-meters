#include "spectrum.h"
#include <algorithm>
#include <cmath>

namespace pm
{
	spectrum::spectrum( ) : meter_panel( "Spectrum" ), fft_( k_fft_size_4096 )
	{
		mono_buffer_.resize( k_fft_size_4096 );
	}

	void spectrum::set_fft_size( size_t size )
	{
		fft_.set_fft_size( size );
		mono_buffer_.resize( size );
	}

	void spectrum::update( const sample_t* samples, size_t frame_count, int channels )
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
	}

	// convert display position (0-1) to frequency using logarithmic scale
	static float position_to_freq( float pos )
	{
		// logarithmic interpolation between min and max frequency
		return k_min_freq * std::pow( k_max_freq / k_min_freq, pos );
	}

	// convert frequency to FFT bin index
	static size_t freq_to_bin( float freq, size_t bin_count, float sample_rate )
	{
		float bin_width = sample_rate / ( bin_count * 2.0f );
		size_t bin      = static_cast< size_t >( freq / bin_width );
		return std::min( bin, bin_count - 1 );
	}

	// get averaged magnitude for a frequency range
	float spectrum::get_band_magnitude_db( float freq_start, float freq_end ) const
	{
		size_t bin_count  = fft_.get_bin_count( );
		float sample_rate = static_cast< float >( fft_.get_sample_rate( ) );

		size_t bin_start = freq_to_bin( freq_start, bin_count, sample_rate );
		size_t bin_end   = freq_to_bin( freq_end, bin_count, sample_rate );

		if ( bin_start >= bin_end )
			bin_end = bin_start + 1;
		if ( bin_end > bin_count )
			bin_end = bin_count;

		// average the magnitudes (in linear domain, then convert to dB)
		float sum = 0.0f;
		for ( size_t i = bin_start; i < bin_end; ++i ) {
			float mag = fft_.get_magnitude( i );
			sum += mag * mag; // sum of squared magnitudes (power)
		}

		float avg_power = sum / ( bin_end - bin_start );
		float avg_mag   = std::sqrt( avg_power );

		return ( avg_mag > 1e-10f ) ? 20.0f * std::log10( avg_mag ) : -100.0f;
	}

	void spectrum::render( )
	{
		ImVec2 canvas_pos  = ImGui::GetCursorScreenPos( );
		ImVec2 canvas_size = ImGui::GetContentRegionAvail( );

		if ( canvas_size.x < 50 || canvas_size.y < 50 )
			return;

		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		// background
		draw_list->AddRectFilled( canvas_pos, ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ), IM_COL32( 15, 15, 20, 255 ) );

		// draw frequency grid lines
		draw_grid( draw_list, canvas_pos, canvas_size );

		switch ( mode_ ) {
		case spectrum_mode::bars:
			draw_bars( draw_list, canvas_pos, canvas_size );
			break;
		case spectrum_mode::lines:
			draw_lines( draw_list, canvas_pos, canvas_size );
			break;
		case spectrum_mode::filled:
			draw_filled( draw_list, canvas_pos, canvas_size );
			break;
		}

		ImGui::Dummy( canvas_size );
	}

	void spectrum::draw_grid( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		// frequency markers at 100Hz, 1kHz, 10kHz
		float freqs[]        = { 100.0f, 1000.0f, 10000.0f };
		const char* labels[] = { "100", "1k", "10k" };

		for ( int i = 0; i < 3; ++i ) {
			float t = std::log( freqs[ i ] / k_min_freq ) / std::log( k_max_freq / k_min_freq );
			float x = pos.x + t * size.x;

			draw_list->AddLine( ImVec2( x, pos.y ), ImVec2( x, pos.y + size.y ), IM_COL32( 50, 50, 60, 255 ) );
			draw_list->AddText( ImVec2( x + 2, pos.y + size.y - 14 ), IM_COL32( 100, 100, 120, 255 ), labels[ i ] );
		}

		// dB markers
		for ( int db = -50; db <= 0; db += 10 ) {
			float normalized = ( db - min_db_ ) / ( max_db_ - min_db_ );
			float y          = pos.y + size.y - normalized * size.y;

			draw_list->AddLine( ImVec2( pos.x, y ), ImVec2( pos.x + size.x, y ), IM_COL32( 40, 40, 50, 255 ) );
		}
	}

	void spectrum::draw_bars( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		const int display_bars = 80;
		float bar_width        = size.x / display_bars;

		for ( int i = 0; i < display_bars; ++i ) {
			// calculate frequency range for this bar
			float t0    = static_cast< float >( i ) / display_bars;
			float t1    = static_cast< float >( i + 1 ) / display_bars;
			float freq0 = position_to_freq( t0 );
			float freq1 = position_to_freq( t1 );

			float db         = get_band_magnitude_db( freq0, freq1 );
			float normalized = ( db - min_db_ ) / ( max_db_ - min_db_ );
			normalized       = std::max( 0.0f, std::min( 1.0f, normalized ) );

			float bar_height = normalized * size.y;
			float x          = pos.x + i * bar_width;
			float y          = pos.y + size.y - bar_height;

			ImU32 color = color_from_db( db, min_db_, max_db_ );

			draw_list->AddRectFilled( ImVec2( x + 1, y ), ImVec2( x + bar_width - 1, pos.y + size.y ), color );
		}
	}

	void spectrum::draw_lines( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		const int points_count = 256;
		std::vector< ImVec2 > points;
		points.reserve( points_count );

		for ( int i = 0; i < points_count; ++i ) {
			float t         = static_cast< float >( i ) / ( points_count - 1 );
			float freq      = position_to_freq( t );
			float freq_next = position_to_freq( t + 1.0f / points_count );

			float db         = get_band_magnitude_db( freq, freq_next );
			float normalized = ( db - min_db_ ) / ( max_db_ - min_db_ );
			normalized       = std::max( 0.0f, std::min( 1.0f, normalized ) );

			float x = pos.x + t * size.x;
			float y = pos.y + size.y - normalized * size.y;

			points.push_back( ImVec2( x, y ) );
		}

		if ( points.size( ) >= 2 ) {
			draw_list->AddPolyline( points.data( ), static_cast< int >( points.size( ) ), IM_COL32( 80, 220, 140, 255 ), ImDrawFlags_None, 2.0f );
		}
	}

	void spectrum::draw_filled( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		const int points_count = 256;

		// draw as series of quads for gradient effect
		for ( int i = 0; i < points_count - 1; ++i ) {
			float t0 = static_cast< float >( i ) / ( points_count - 1 );
			float t1 = static_cast< float >( i + 1 ) / ( points_count - 1 );

			float freq0 = position_to_freq( t0 );
			float freq1 = position_to_freq( t1 );

			float db0 = get_band_magnitude_db( freq0, freq1 );
			float db1 = get_band_magnitude_db( freq1, position_to_freq( t1 + 1.0f / points_count ) );

			float norm0 = std::max( 0.0f, std::min( 1.0f, ( db0 - min_db_ ) / ( max_db_ - min_db_ ) ) );
			float norm1 = std::max( 0.0f, std::min( 1.0f, ( db1 - min_db_ ) / ( max_db_ - min_db_ ) ) );

			float x0       = pos.x + t0 * size.x;
			float x1       = pos.x + t1 * size.x;
			float y0       = pos.y + size.y - norm0 * size.y;
			float y1       = pos.y + size.y - norm1 * size.y;
			float y_bottom = pos.y + size.y;

			ImU32 col0 = color_from_db( db0, min_db_, max_db_ );
			ImU32 col1 = color_from_db( db1, min_db_, max_db_ );

			ImU32 fill0 = ( col0 & 0x00FFFFFF ) | 0x60000000;
			ImU32 fill1 = ( col1 & 0x00FFFFFF ) | 0x60000000;

			draw_list->AddQuadFilled( ImVec2( x0, y0 ), ImVec2( x1, y1 ), ImVec2( x1, y_bottom ), ImVec2( x0, y_bottom ), fill0 );

			// top line
			draw_list->AddLine( ImVec2( x0, y0 ), ImVec2( x1, y1 ), col0, 1.5f );
		}
	}

} // namespace pm
