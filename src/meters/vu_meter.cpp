#include "vu_meter.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace pm
{

	vu_meter::vu_meter( ) : meter_panel( "VU Meter" )
	{
		// VU meter integration time ~300ms
		// assuming ~60fps updates
		integration_coeff_ = 1.0f - std::exp( -1.0f / ( 0.3f * 60.0f ) );
	}

	void vu_meter::update( const sample_t* samples, size_t frame_count, int channels )
	{
		if ( channels < 1 )
			return;

		// calculate RMS
		float sum_l = 0.0f, sum_r = 0.0f;
		float max_l = 0.0f, max_r = 0.0f;

		for ( size_t i = 0; i < frame_count; ++i ) {
			float l = samples[ i * channels ];
			sum_l += l * l;
			max_l = std::max( max_l, std::abs( l ) );

			if ( channels >= 2 ) {
				float r = samples[ i * channels + 1 ];
				sum_r += r * r;
				max_r = std::max( max_r, std::abs( r ) );
			} else {
				sum_r = sum_l;
				max_r = max_l;
			}
		}

		float rms_l = std::sqrt( sum_l / frame_count );
		float rms_r = std::sqrt( sum_r / frame_count );

		// convert to dB
		float db_l      = ( rms_l > 1e-10f ) ? 20.0f * std::log10( rms_l ) : -60.0f;
		float db_r      = ( rms_r > 1e-10f ) ? 20.0f * std::log10( rms_r ) : -60.0f;
		float peak_db_l = ( max_l > 1e-10f ) ? 20.0f * std::log10( max_l ) : -60.0f;
		float peak_db_r = ( max_r > 1e-10f ) ? 20.0f * std::log10( max_r ) : -60.0f;

		// apply VU ballistics (slow integration)
		vu_l_ += ( db_l - vu_l_ ) * integration_coeff_;
		vu_r_ += ( db_r - vu_r_ ) * integration_coeff_;

		// peak with fast attack, slow decay
		if ( peak_db_l > peak_l_ ) {
			peak_l_ = peak_db_l;
		} else {
			peak_l_ -= 0.3f; // decay ~18dB/sec at 60fps
		}

		if ( peak_db_r > peak_r_ ) {
			peak_r_ = peak_db_r;
		} else {
			peak_r_ -= 0.3f;
		}
	}

	void vu_meter::render( )
	{
		ImVec2 canvas_pos  = ImGui::GetCursorScreenPos( );
		ImVec2 canvas_size = ImGui::GetContentRegionAvail( );

		if ( canvas_size.x < 100 || canvas_size.y < 80 )
			return;

		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		float meter_width = ( canvas_size.x - 20 ) / 2.0f;
		float radius      = std::min( meter_width * 0.8f, canvas_size.y * 0.6f );

		ImVec2 center_l( canvas_pos.x + meter_width * 0.5f + 5, canvas_pos.y + canvas_size.y - 20 );
		ImVec2 center_r( canvas_pos.x + meter_width * 1.5f + 15, canvas_pos.y + canvas_size.y - 20 );

		draw_vu_arc( draw_list, center_l, radius, vu_l_, peak_l_, true );
		draw_vu_arc( draw_list, center_r, radius, vu_r_, peak_r_, false );

		ImGui::Dummy( canvas_size );
	}

	void vu_meter::draw_vu_arc( ImDrawList* draw_list, ImVec2 center, float radius, float value_vu, float peak_vu, bool is_left )
	{
		const float min_vu      = -20.0f;
		const float max_vu      = 3.0f;
		const float angle_start = std::numbers::pi_v< float > * 0.75f; // 135 degrees
		const float angle_end   = std::numbers::pi_v< float > * 0.25f; // 45 degrees
		const float angle_range = angle_start - angle_end;

		// draw arc background
		draw_list->PathArcTo( center, radius, angle_end, angle_start, 32 );
		draw_list->PathStroke( IM_COL32( 60, 60, 70, 255 ), ImDrawFlags_None, 4.0f );

		// draw scale markings
		for ( int vu = -20; vu <= 3; vu += 1 ) {
			float t     = ( vu - min_vu ) / ( max_vu - min_vu );
			float angle = angle_start - t * angle_range;

			float mark_len = ( vu % 5 == 0 ) ? 15.0f : 8.0f;
			float inner_r  = radius - mark_len;

			ImVec2 p1( center.x + std::cos( angle ) * inner_r, center.y - std::sin( angle ) * inner_r );
			ImVec2 p2( center.x + std::cos( angle ) * radius, center.y - std::sin( angle ) * radius );

			ImU32 color = ( vu >= 0 ) ? IM_COL32( 200, 80, 80, 255 ) : IM_COL32( 150, 150, 150, 255 );
			draw_list->AddLine( p1, p2, color, ( vu % 5 == 0 ) ? 2.0f : 1.0f );

			// labels for major marks
			if ( vu % 5 == 0 || vu == 0 || vu == 3 ) {
				char buf[ 8 ];
				snprintf( buf, sizeof( buf ), "%d", vu );
				ImVec2 text_pos( center.x + std::cos( angle ) * ( radius + 12 ) - 8, center.y - std::sin( angle ) * ( radius + 12 ) - 6 );
				draw_list->AddText( text_pos, color, buf );
			}
		}

		// draw needle
		float vu_value     = value_vu + calibration_db_; // apply calibration
		vu_value           = std::max( min_vu, std::min( max_vu, vu_value ) );
		float t            = ( vu_value - min_vu ) / ( max_vu - min_vu );
		float needle_angle = angle_start - t * angle_range;

		ImVec2 needle_end( center.x + std::cos( needle_angle ) * ( radius * 0.9f ), center.y - std::sin( needle_angle ) * ( radius * 0.9f ) );

		draw_list->AddLine( center, needle_end, IM_COL32( 220, 220, 220, 255 ), 2.0f );
		draw_list->AddCircleFilled( center, 8.0f, IM_COL32( 80, 80, 90, 255 ) );

		// channel label
		const char* label = is_left ? "L" : "R";
		draw_list->AddText( ImVec2( center.x - 4, center.y - radius - 25 ), IM_COL32( 200, 200, 200, 255 ), label );
	}

} // namespace pm
