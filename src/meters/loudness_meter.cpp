// Loudness meter implementation

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

		// peak hold
		if ( peak_l_ > peak_hold_l_ ) {
			peak_hold_l_     = peak_l_;
			peak_hold_timer_ = 60; // hold for ~1 second at 60fps
		}
		if ( peak_r_ > peak_hold_r_ ) {
			peak_hold_r_     = peak_r_;
			peak_hold_timer_ = 60;
		}

		if ( peak_hold_timer_ > 0 ) {
			peak_hold_timer_--;
		} else {
			peak_hold_l_ = std::max( peak_hold_l_ - 0.5f, peak_l_ );
			peak_hold_r_ = std::max( peak_hold_r_ - 0.5f, peak_r_ );
		}

		// LUFS
		if ( channels >= 2 ) {
			lufs_.process( samples, frame_count );
		}
	}

	void loudness_meter::render( )
	{
		ImVec2 canvas_size    = ImGui::GetContentRegionAvail( );
		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		float bar_height = 25.0f;
		float spacing    = 8.0f;

		ImVec2 cursor = ImGui::GetCursorScreenPos( );

		// L
		draw_meter_bar( draw_list, cursor, ImVec2( canvas_size.x, bar_height ), peak_l_, peak_hold_l_, "L" );
		cursor.y += bar_height + spacing;

		// R
		draw_meter_bar( draw_list, cursor, ImVec2( canvas_size.x, bar_height ), peak_r_, peak_hold_r_, "R" );
		cursor.y += bar_height + spacing * 2;

		// LUFS values
		ImGui::SetCursorScreenPos( cursor );
		ImGui::Text( "LUFS M: %.1f  S: %.1f  I: %.1f", lufs_.get_momentary( ), lufs_.get_short_term( ), lufs_.get_integrated( ) );

		ImGui::Text( "Peak L: %.1f dB  R: %.1f dB", peak_l_, peak_r_ );
		ImGui::Text( "RMS  L: %.1f dB  R: %.1f dB", rms_l_, rms_r_ );
	}

	void loudness_meter::draw_meter_bar( ImDrawList* draw_list, ImVec2 pos, ImVec2 size, float value_db, float peak_db, const char* label )
	{
		const float min_db = -60.0f;
		const float max_db = 0.0f;

		// background
		draw_list->AddRectFilled( pos, ImVec2( pos.x + size.x, pos.y + size.y ), IM_COL32( 30, 30, 35, 255 ) );

		// value bar
		float normalized = ( value_db - min_db ) / ( max_db - min_db );
		normalized       = std::max( 0.0f, std::min( 1.0f, normalized ) );
		float bar_width  = normalized * ( size.x - 30.0f );

		ImU32 bar_color = color_from_db( value_db, min_db, max_db );
		draw_list->AddRectFilled( ImVec2( pos.x + 25.0f, pos.y + 2 ), ImVec2( pos.x + 25.0f + bar_width, pos.y + size.y - 2 ), bar_color );

		// peak hold marker
		float peak_normalized = ( peak_db - min_db ) / ( max_db - min_db );
		peak_normalized       = std::max( 0.0f, std::min( 1.0f, peak_normalized ) );
		float peak_x          = pos.x + 25.0f + peak_normalized * ( size.x - 30.0f );

		draw_list->AddLine( ImVec2( peak_x, pos.y + 2 ), ImVec2( peak_x, pos.y + size.y - 2 ), IM_COL32( 255, 255, 255, 200 ), 2.0f );

		// label
		draw_list->AddText( ImVec2( pos.x + 5, pos.y + 4 ), IM_COL32( 200, 200, 200, 255 ), label );
	}

} // namespace pm
