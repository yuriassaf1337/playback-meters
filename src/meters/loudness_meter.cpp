#include "loudness_meter.h"
#include <algorithm>
#include <cmath>

namespace pm
{

	loudness_meter::loudness_meter( ) : meter_panel( "Loudness" ) { }

	void loudness_meter::update( const sample_t* samples, size_t frame_count, int channels )
	{
		if ( channels < 1 )
			return;

		// calculate peak and RMS for each channel
		float max_l = 0.0f, max_r = 0.0f;
		float sum_l = 0.0f, sum_r = 0.0f;

		for ( size_t i = 0; i < frame_count; ++i ) {
			float l = std::abs( samples[ i * channels ] );
			max_l   = std::max( max_l, l );
			sum_l += l * l;

			if ( channels >= 2 ) {
				float r = std::abs( samples[ i * channels + 1 ] );
				max_r   = std::max( max_r, r );
				sum_r += r * r;
			} else {
				max_r = max_l;
				sum_r = sum_l;
			}
		}

		// convert to dB
		peak_l_ = ( max_l > 1e-10f ) ? 20.0f * std::log10( max_l ) : -100.0f;
		peak_r_ = ( max_r > 1e-10f ) ? 20.0f * std::log10( max_r ) : -100.0f;

		float rms_val_l = std::sqrt( sum_l / frame_count );
		float rms_val_r = std::sqrt( sum_r / frame_count );
		rms_l_          = ( rms_val_l > 1e-10f ) ? 20.0f * std::log10( rms_val_l ) : -100.0f;
		rms_r_          = ( rms_val_r > 1e-10f ) ? 20.0f * std::log10( rms_val_r ) : -100.0f;

		// peak hold (combined mono peak)
		float combined_peak = std::max( peak_l_, peak_r_ );
		if ( combined_peak > peak_hold_ ) {
			peak_hold_       = combined_peak;
			peak_hold_timer_ = 60; // hold for ~1 second
		}

		if ( peak_hold_timer_ > 0 ) {
			peak_hold_timer_--;
		} else {
			peak_hold_ = std::max( peak_hold_ - 0.5f, combined_peak );
		}

		// accumulate RMS for fast (0.3s) and slow (1.0s) windows
		// assuming ~48kHz sample rate and ~1024 frame buffers = ~46 updates/sec
		// 0.3s = ~14 updates, 1.0s = ~46 updates
		float combined_rms_linear = ( rms_val_l + rms_val_r ) * 0.5f;
		float combined_rms_sq     = combined_rms_linear * combined_rms_linear;

		rms_fast_sum_ += combined_rms_sq;
		rms_fast_count_++;
		rms_slow_sum_ += combined_rms_sq;
		rms_slow_count_++;

		// (approx 0.3s)
		if ( rms_fast_count_ >= 14 ) {
			float avg       = rms_fast_sum_ / static_cast< float >( rms_fast_count_ );
			rms_fast_       = ( avg > 1e-20f ) ? 10.0f * std::log10( avg ) : -100.0f;
			rms_fast_sum_   = 0.0f;
			rms_fast_count_ = 0;
		}

		// (approx 1.0s)
		if ( rms_slow_count_ >= 46 ) {
			float avg       = rms_slow_sum_ / static_cast< float >( rms_slow_count_ );
			rms_slow_       = ( avg > 1e-20f ) ? 10.0f * std::log10( avg ) : -100.0f;
			rms_slow_sum_   = 0.0f;
			rms_slow_count_ = 0;
		}

		// LUFS processing
		if ( channels >= 2 ) {
			lufs_.process( samples, frame_count );
		}
	}

	float loudness_meter::get_display_value( ) const
	{
		switch ( mode_ ) {
		case loudness_mode::lufs_momentary:
			return lufs_.get_momentary( );
		case loudness_mode::lufs_short:
			return lufs_.get_short_term( );
		case loudness_mode::rms_fast:
			return rms_fast_;
		case loudness_mode::rms_slow:
			return rms_slow_;
		default:
			return lufs_.get_momentary( );
		}
	}

	const char* loudness_meter::get_mode_label( ) const
	{
		switch ( mode_ ) {
		case loudness_mode::lufs_momentary:
			return "LUFS M";
		case loudness_mode::lufs_short:
			return "LUFS S";
		case loudness_mode::rms_fast:
			return "RMS Fast";
		case loudness_mode::rms_slow:
			return "RMS Slow";
		default:
			return "LUFS M";
		}
	}

	void loudness_meter::render( )
	{
		ImVec2 canvas_size    = ImGui::GetContentRegionAvail( );
		ImDrawList* draw_list = ImGui::GetWindowDrawList( );
		ImVec2 cursor         = ImGui::GetCursorScreenPos( );

		const char* mode_items[] = { "LUFS Momentary", "LUFS Short-Term", "RMS Fast (0.3s)", "RMS Slow (1.0s)" };
		int current_mode         = static_cast< int >( mode_ );
		ImGui::SetNextItemWidth( canvas_size.x );
		if ( ImGui::Combo( "##loudness_mode", &current_mode, mode_items, 4 ) ) {
			mode_ = static_cast< loudness_mode >( current_mode );
		}

		canvas_size = ImGui::GetContentRegionAvail( );
		cursor      = ImGui::GetCursorScreenPos( );

		float meter_width = 40.0f;
		float padding     = 10.0f;
		float value_width = canvas_size.x - meter_width - padding * 2;

		ImVec2 meter_pos  = cursor;
		ImVec2 meter_size = ImVec2( meter_width, canvas_size.y - 10.0f );
		draw_vertical_meter( draw_list, meter_pos, meter_size );

		float display_value = get_display_value( );
		ImVec2 value_pos    = ImVec2( cursor.x + meter_width + padding, cursor.y );

		char value_text[ 32 ];
		if ( display_value <= -100.0f ) {
			snprintf( value_text, sizeof( value_text ), "-inf" );
		} else {
			snprintf( value_text, sizeof( value_text ), "%.1f", display_value );
		}

		draw_list->AddText( ImVec2( value_pos.x, value_pos.y ), IM_COL32( 150, 150, 150, 255 ), get_mode_label( ) );

		ImVec2 large_text_pos = ImVec2( value_pos.x, value_pos.y + 20.0f );

		ImU32 value_color = IM_COL32( 255, 255, 255, 255 );
		if ( display_value > -6.0f ) {
			value_color = IM_COL32( 255, 200, 50, 255 ); // yellow for high values
		} else if ( display_value > -14.0f ) {
			value_color = IM_COL32( 100, 255, 100, 255 ); // green for normal
		}

		for ( int dx = 0; dx <= 1; ++dx ) {
			for ( int dy = 0; dy <= 1; ++dy ) {
				draw_list->AddText( ImVec2( large_text_pos.x + dx, large_text_pos.y + dy ), value_color, value_text );
			}
		}

		// draw unit label
		const char* unit = ( mode_ == loudness_mode::lufs_momentary || mode_ == loudness_mode::lufs_short ) ? "LUFS" : "dB";
		draw_list->AddText( ImVec2( value_pos.x, value_pos.y + 40.0f ), IM_COL32( 150, 150, 150, 255 ), unit );

		// draw peak hold value
		char peak_text[ 32 ];
		snprintf( peak_text, sizeof( peak_text ), "Peak: %.1f dB", peak_hold_ );
		draw_list->AddText( ImVec2( value_pos.x, value_pos.y + 60.0f ), IM_COL32( 200, 200, 200, 255 ), peak_text );

		// draw integrated LUFS if in LUFS mode
		if ( mode_ == loudness_mode::lufs_momentary || mode_ == loudness_mode::lufs_short ) {
			char int_text[ 32 ];
			snprintf( int_text, sizeof( int_text ), "Int: %.1f LUFS", lufs_.get_integrated( ) );
			draw_list->AddText( ImVec2( value_pos.x, value_pos.y + 80.0f ), IM_COL32( 180, 180, 180, 255 ), int_text );
		}

		// advance cursor
		ImGui::Dummy( canvas_size );
	}

	void loudness_meter::draw_vertical_meter( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		const float min_db = -60.0f;
		const float max_db = 0.0f;

		// background
		draw_list->AddRectFilled( pos, ImVec2( pos.x + size.x, pos.y + size.y ), IM_COL32( 25, 25, 30, 255 ), 3.0f );

		float value_db = get_display_value( );

		float normalized = ( value_db - min_db ) / ( max_db - min_db );
		normalized       = std::max( 0.0f, std::min( 1.0f, normalized ) );

		float bar_height = normalized * ( size.y - 4.0f );
		float bar_top    = pos.y + size.y - 2.0f - bar_height;
		float bar_bottom = pos.y + size.y - 2.0f;

		ImU32 bar_color_top, bar_color_bottom;

		if ( value_db > -6.0f ) {
			// red/yellow for high levels
			bar_color_top    = IM_COL32( 255, 80, 80, 255 );
			bar_color_bottom = IM_COL32( 255, 180, 50, 255 );
		} else if ( value_db > -18.0f ) {
			// yellow/green for mid levels
			bar_color_top    = IM_COL32( 255, 200, 50, 255 );
			bar_color_bottom = IM_COL32( 80, 220, 80, 255 );
		} else {
			// green/blue for low levels
			bar_color_top    = IM_COL32( 80, 220, 80, 255 );
			bar_color_bottom = IM_COL32( 50, 150, 200, 255 );
		}

		// draw gradient bar
		draw_list->AddRectFilledMultiColor( ImVec2( pos.x + 2.0f, bar_top ), ImVec2( pos.x + size.x - 2.0f, bar_bottom ), bar_color_top,
		                                    bar_color_top, bar_color_bottom, bar_color_bottom );

		// peak hold line
		float peak_normalized = ( peak_hold_ - min_db ) / ( max_db - min_db );
		peak_normalized       = std::max( 0.0f, std::min( 1.0f, peak_normalized ) );
		float peak_y          = pos.y + size.y - 2.0f - peak_normalized * ( size.y - 4.0f );

		draw_list->AddLine( ImVec2( pos.x + 2.0f, peak_y ), ImVec2( pos.x + size.x - 2.0f, peak_y ), IM_COL32( 255, 255, 255, 200 ), 2.0f );

		// scale markers
		const float markers[] = { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f };
		for ( float db : markers ) {
			float marker_norm = ( db - min_db ) / ( max_db - min_db );
			float marker_y    = pos.y + size.y - 2.0f - marker_norm * ( size.y - 4.0f );

			// tick mark
			draw_list->AddLine( ImVec2( pos.x + size.x - 8.0f, marker_y ), ImVec2( pos.x + size.x - 2.0f, marker_y ), IM_COL32( 100, 100, 100, 255 ),
			                    1.0f );
		}

		// border
		draw_list->AddRect( pos, ImVec2( pos.x + size.x, pos.y + size.y ), IM_COL32( 60, 60, 70, 255 ), 3.0f );
	}

} // namespace pm
