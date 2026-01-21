#include "spectrum.h"
#include "../dsp/note_utils.h"
#include <algorithm>
#include <cmath>

namespace pm
{

	spectrum::spectrum( ) : meter_panel( "Spectrum" ), fft_( k_fft_size_4096 )
	{
		left_buffer_.resize( k_fft_size_4096 );
		right_buffer_.resize( k_fft_size_4096 );
	}

	void spectrum::set_fft_size( size_t size )
	{
		fft_.set_fft_size( size );
		left_buffer_.resize( size );
		right_buffer_.resize( size );
	}

	void spectrum::update( const sample_t* samples, size_t frame_count, int channels )
	{
		size_t copy_count = std::min( frame_count, left_buffer_.size( ) );

		// extract channels
		for ( size_t i = 0; i < copy_count; ++i ) {
			float l            = ( channels >= 1 ) ? samples[ i * channels ] : 0.0f;
			float r            = ( channels >= 2 ) ? samples[ i * channels + 1 ] : l;
			left_buffer_[ i ]  = l;
			right_buffer_[ i ] = r;
		}

		// select channel to process
		std::vector< sample_t >* buffer = &left_buffer_;
		switch ( channel_ ) {
		case spectrum_channel::left:
			buffer = &left_buffer_;
			break;
		case spectrum_channel::right:
			buffer = &right_buffer_;
			break;
		case spectrum_channel::mid:
			for ( size_t i = 0; i < copy_count; ++i ) {
				left_buffer_[ i ] = ( left_buffer_[ i ] + right_buffer_[ i ] ) * 0.5f;
			}
			buffer = &left_buffer_;
			break;
		case spectrum_channel::side:
			for ( size_t i = 0; i < copy_count; ++i ) {
				left_buffer_[ i ] = ( left_buffer_[ i ] - right_buffer_[ i ] ) * 0.5f;
			}
			buffer = &left_buffer_;
			break;
		}

		fft_.process( buffer->data( ), copy_count );
	}

	float spectrum::position_to_freq( float pos ) const
	{
		pos = std::max( 0.0f, std::min( 1.0f, pos ) );

		switch ( scale_ ) {
		case spectrum_scale::linear:
			return k_min_freq + pos * ( k_max_freq - k_min_freq );

		case spectrum_scale::logarithmic:
			return k_min_freq * std::pow( k_max_freq / k_min_freq, pos );

		case spectrum_scale::mel: {
			// mel scale: more resolution in speech range
			float mel_min = 2595.0f * std::log10( 1.0f + k_min_freq / 700.0f );
			float mel_max = 2595.0f * std::log10( 1.0f + k_max_freq / 700.0f );
			float mel     = mel_min + pos * ( mel_max - mel_min );
			return 700.0f * ( std::pow( 10.0f, mel / 2595.0f ) - 1.0f );
		}
		}
		return k_min_freq;
	}

	float spectrum::freq_to_position( float freq ) const
	{
		freq = std::max( k_min_freq, std::min( k_max_freq, freq ) );

		switch ( scale_ ) {
		case spectrum_scale::linear:
			return ( freq - k_min_freq ) / ( k_max_freq - k_min_freq );

		case spectrum_scale::logarithmic:
			return std::log( freq / k_min_freq ) / std::log( k_max_freq / k_min_freq );

		case spectrum_scale::mel: {
			float mel_min  = 2595.0f * std::log10( 1.0f + k_min_freq / 700.0f );
			float mel_max  = 2595.0f * std::log10( 1.0f + k_max_freq / 700.0f );
			float mel_freq = 2595.0f * std::log10( 1.0f + freq / 700.0f );
			return ( mel_freq - mel_min ) / ( mel_max - mel_min );
		}
		}
		return 0.0f;
	}

	float spectrum::get_band_magnitude_db( float freq_start, float freq_end ) const
	{
		size_t bin_count  = fft_.get_bin_count( );
		float sample_rate = static_cast< float >( fft_.get_sample_rate( ) );
		float bin_width   = sample_rate / ( bin_count * 2.0f );

		size_t bin_start = static_cast< size_t >( freq_start / bin_width );
		size_t bin_end   = static_cast< size_t >( freq_end / bin_width );

		bin_start = std::min( bin_start, bin_count - 1 );
		bin_end   = std::min( bin_end, bin_count );

		if ( bin_start >= bin_end )
			bin_end = bin_start + 1;
		if ( bin_end > bin_count )
			bin_end = bin_count;

		float sum = 0.0f;
		for ( size_t i = bin_start; i < bin_end; ++i ) {
			float mag = fft_.get_magnitude( i );
			sum += mag * mag;
		}

		float avg_power = sum / std::max( 1.0f, static_cast< float >( bin_end - bin_start ) );
		float avg_mag   = std::sqrt( avg_power );

		return ( avg_mag > 1e-10f ) ? 20.0f * std::log10( avg_mag ) : -100.0f;
	}

	void spectrum::find_peak( ImVec2 pos, ImVec2 size )
	{
		peak_.db        = -100.0f;
		peak_.frequency = 0.0f;

		size_t bin_count  = fft_.get_bin_count( );
		float sample_rate = static_cast< float >( fft_.get_sample_rate( ) );

		for ( size_t i = 1; i < bin_count; ++i ) {
			float db = fft_.get_magnitude_db( i );
			if ( db > peak_.db ) {
				peak_.db        = db;
				peak_.frequency = fft_.get_frequency( i );
			}
		}

		// calculate screen pos
		float t    = freq_to_position( peak_.frequency );
		peak_.x    = pos.x + t * size.x;
		float norm = ( peak_.db - min_db_ ) / ( max_db_ - min_db_ );
		norm       = std::max( 0.0f, std::min( 1.0f, norm ) );
		peak_.y    = pos.y + size.y - norm * size.y;
	}

	ImU32 spectrum::get_bar_color( float t ) const
	{
		if ( t < 0.33f ) {
			float lt = t / 0.33f;
			return lerp_color( IM_COL32( 255, 100, 50, 255 ), IM_COL32( 255, 80, 120, 255 ), lt );
		} else if ( t < 0.66f ) {
			float lt = ( t - 0.33f ) / 0.33f;
			return lerp_color( IM_COL32( 255, 80, 120, 255 ), IM_COL32( 200, 80, 180, 255 ), lt );
		} else {
			float lt = ( t - 0.66f ) / 0.34f;
			return lerp_color( IM_COL32( 200, 80, 180, 255 ), IM_COL32( 150, 100, 220, 255 ), lt );
		}
	}

	void spectrum::render( )
	{
		ImVec2 canvas_pos  = ImGui::GetCursorScreenPos( );
		ImVec2 canvas_size = ImGui::GetContentRegionAvail( );

		if ( canvas_size.x < 50 || canvas_size.y < 50 )
			return;

		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		// background
		draw_list->AddRectFilled( canvas_pos, ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ), IM_COL32( 12, 12, 16, 255 ) );

		// grid
		draw_grid( draw_list, canvas_pos, canvas_size );

		// find peak for tooltip
		find_peak( canvas_pos, canvas_size );

		// draw based on mode
		if ( display_mode_ == spectrum_display_mode::color_bars || display_mode_ == spectrum_display_mode::both ) {
			draw_color_bars( draw_list, canvas_pos, canvas_size );
		}
		if ( display_mode_ == spectrum_display_mode::fft || display_mode_ == spectrum_display_mode::both ) {
			draw_fft_line( draw_list, canvas_pos, canvas_size );
		}

		// peak tooltip
		if ( show_peak_info_ && peak_.db > min_db_ + 10.0f ) {
			draw_peak_tooltip( draw_list, canvas_pos, canvas_size );
		}

		ImGui::Dummy( canvas_size );
	}

	void spectrum::draw_grid( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		// frequency markers
		float freqs[]        = { 100.0f, 1000.0f, 10000.0f };
		const char* labels[] = { "100Hz", "1kHz", "10kHz" };

		for ( int i = 0; i < 3; ++i ) {
			float t = freq_to_position( freqs[ i ] );
			float x = pos.x + t * size.x;

			draw_list->AddLine( ImVec2( x, pos.y ), ImVec2( x, pos.y + size.y ), IM_COL32( 40, 40, 50, 255 ) );
			draw_list->AddText( ImVec2( x + 2, pos.y + 2 ), IM_COL32( 80, 80, 100, 255 ), labels[ i ] );
		}

		// db markers
		for ( int db = -50; db <= 0; db += 10 ) {
			float normalized = ( db - min_db_ ) / ( max_db_ - min_db_ );
			float y          = pos.y + size.y - normalized * size.y;
			draw_list->AddLine( ImVec2( pos.x, y ), ImVec2( pos.x + size.x, y ), IM_COL32( 30, 30, 40, 255 ) );
		}
	}

	void spectrum::draw_color_bars( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		const int bar_count = 128;
		float bar_width     = size.x / bar_count;

		for ( int i = 0; i < bar_count; ++i ) {
			float t0    = static_cast< float >( i ) / bar_count;
			float t1    = static_cast< float >( i + 1 ) / bar_count;
			float freq0 = position_to_freq( t0 );
			float freq1 = position_to_freq( t1 );

			float db         = get_band_magnitude_db( freq0, freq1 );
			float normalized = ( db - min_db_ ) / ( max_db_ - min_db_ );
			normalized       = std::max( 0.0f, std::min( 1.0f, normalized ) );

			if ( normalized < 0.01f )
				continue;

			float bar_height = normalized * size.y;
			float x          = pos.x + i * bar_width;
			float y_top      = pos.y + size.y - bar_height;
			float y_bottom   = pos.y + size.y;

			// gradient fill
			ImU32 color_top    = get_bar_color( t0 );
			ImU32 color_bottom = ( color_top & 0x00FFFFFF ) | 0x40000000;

			draw_list->AddRectFilledMultiColor( ImVec2( x, y_top ), ImVec2( x + bar_width - 1, y_bottom ), color_top, color_top, color_bottom,
			                                    color_bottom );
		}
	}

	void spectrum::draw_fft_line( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
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
			draw_list->AddPolyline( points.data( ), static_cast< int >( points.size( ) ), IM_COL32( 220, 220, 240, 200 ), ImDrawFlags_None, 1.5f );
		}
	}

	void spectrum::draw_peak_tooltip( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		std::string note_str = freq_to_note_string( peak_.frequency );

		char buf[ 64 ];
		if ( note_str.empty( ) ) {
			snprintf( buf, sizeof( buf ), "%.1fdB | %.1fHz", peak_.db, peak_.frequency );
		} else {
			snprintf( buf, sizeof( buf ), "%.1fdB | %.1fHz | %s", peak_.db, peak_.frequency, note_str.c_str( ) );
		}

		// tooltip background
		ImVec2 text_size = ImGui::CalcTextSize( buf );
		float tooltip_x  = std::max( pos.x + 5, std::min( peak_.x - text_size.x * 0.5f, pos.x + size.x - text_size.x - 10 ) );
		float tooltip_y  = std::max( pos.y + 5, peak_.y - 25 );

		draw_list->AddRectFilled( ImVec2( tooltip_x - 4, tooltip_y - 2 ), ImVec2( tooltip_x + text_size.x + 4, tooltip_y + text_size.y + 2 ),
		                          IM_COL32( 200, 60, 60, 230 ), 3.0f );

		draw_list->AddText( ImVec2( tooltip_x, tooltip_y ), IM_COL32( 255, 255, 255, 255 ), buf );

		// peak marker triangle
		draw_list->AddTriangleFilled( ImVec2( peak_.x, peak_.y ), ImVec2( peak_.x - 6, peak_.y - 10 ), ImVec2( peak_.x + 6, peak_.y - 10 ),
		                              IM_COL32( 200, 60, 60, 255 ) );
	}

} // namespace pm
