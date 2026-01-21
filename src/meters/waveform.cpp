#include "waveform.h"
#include <algorithm>
#include <cmath>

namespace pm
{

	waveform::waveform( ) : meter_panel( "Waveform" )
	{
		history_.resize( k_history_width );
		for ( auto& col : history_ ) {
			col.min_l = col.max_l = 0.0f;
			col.min_r = col.max_r = 0.0f;
			col.rms_l = col.rms_r = 0.0f;
		}
	}

	void waveform::update( const sample_t* samples, size_t frame_count, int channels )
	{
		for ( size_t i = 0; i < frame_count; ++i ) {
			float l = ( channels >= 1 ) ? samples[ i * channels ] : 0.0f;
			float r = ( channels >= 2 ) ? samples[ i * channels + 1 ] : l;

			acc_min_l_ = std::min( acc_min_l_, l );
			acc_max_l_ = std::max( acc_max_l_, l );
			acc_min_r_ = std::min( acc_min_r_, r );
			acc_max_r_ = std::max( acc_max_r_, r );
			acc_sum_l_ += l * l;
			acc_sum_r_ += r * r;
			acc_samples_++;

			// when we have enough samples for one column
			size_t threshold = static_cast< size_t >( samples_per_column_ / scroll_speed_ );
			if ( acc_samples_ >= threshold ) {
				auto& col = history_[ write_pos_ ];
				col.min_l = acc_min_l_;
				col.max_l = acc_max_l_;
				col.min_r = acc_min_r_;
				col.max_r = acc_max_r_;
				col.rms_l = std::sqrt( acc_sum_l_ / acc_samples_ );
				col.rms_r = std::sqrt( acc_sum_r_ / acc_samples_ );

				// update peak history
				float peak                  = std::max( std::abs( col.max_l ), std::abs( col.max_r ) );
				peak                        = std::max( peak, std::max( std::abs( col.min_l ), std::abs( col.min_r ) ) );
				peak_history_[ write_pos_ ] = peak;

				if ( loop_mode_ == waveform_loop_mode::scroll ) {
					write_pos_ = ( write_pos_ + 1 ) % k_history_width;
				} else {
					// static loop: overwrite from start when full
					write_pos_++;
					if ( write_pos_ >= k_history_width )
						write_pos_ = 0;
				}

				acc_min_l_ = acc_max_l_ = 0.0f;
				acc_min_r_ = acc_max_r_ = 0.0f;
				acc_sum_l_ = acc_sum_r_ = 0.0f;
				acc_samples_            = 0;
			}
		}
	}

	ImU32 waveform::get_column_color( size_t idx, float intensity ) const
	{
		switch ( color_mode_ ) {
		case waveform_color_mode::static_color:
			return IM_COL32( 120, 80, 180, 180 );

		case waveform_color_mode::multi_band: {
			// gradient across display (position-based)
			float t = static_cast< float >( idx ) / k_history_width;
			if ( t < 0.33f ) {
				return lerp_color( IM_COL32( 80, 180, 200, 180 ), IM_COL32( 120, 220, 120, 180 ), t / 0.33f );
			} else if ( t < 0.66f ) {
				return lerp_color( IM_COL32( 120, 220, 120, 180 ), IM_COL32( 220, 180, 80, 180 ), ( t - 0.33f ) / 0.33f );
			} else {
				return lerp_color( IM_COL32( 220, 180, 80, 180 ), IM_COL32( 220, 80, 100, 180 ), ( t - 0.66f ) / 0.34f );
			}
		}

		case waveform_color_mode::color_map: {
			// intensity-based (louder = warmer color)
			intensity = std::max( 0.0f, std::min( 1.0f, intensity ) );
			if ( intensity < 0.5f ) {
				return lerp_color( IM_COL32( 50, 80, 150, 180 ), IM_COL32( 80, 180, 200, 180 ), intensity * 2.0f );
			} else {
				return lerp_color( IM_COL32( 80, 180, 200, 180 ), IM_COL32( 255, 100, 80, 220 ), ( intensity - 0.5f ) * 2.0f );
			}
		}
		}
		return IM_COL32( 120, 80, 180, 180 );
	}

	void waveform::draw_peak_history( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		float col_width = size.x / k_history_width;
		float center_y  = pos.y + size.y * 0.5f;
		float scale     = size.y * 0.45f;

		std::vector< ImVec2 > points;
		points.reserve( k_history_width );

		for ( size_t i = 0; i < k_history_width; ++i ) {
			size_t idx = ( write_pos_ + i ) % k_history_width;
			float x    = pos.x + i * col_width;
			float y    = center_y - peak_history_[ idx ] * scale;
			points.push_back( ImVec2( x, y ) );
		}

		if ( points.size( ) >= 2 ) {
			draw_list->AddPolyline( points.data( ), static_cast< int >( points.size( ) ), IM_COL32( 255, 200, 100, 150 ), ImDrawFlags_None, 1.5f );
		}
	}

	void waveform::render( )
	{
		ImVec2 canvas_pos  = ImGui::GetCursorScreenPos( );
		ImVec2 canvas_size = ImGui::GetContentRegionAvail( );

		if ( canvas_size.x < 50 || canvas_size.y < 50 )
			return;

		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		// background
		draw_list->AddRectFilled( canvas_pos, ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ), IM_COL32( 18, 18, 22, 255 ) );

		float center_y    = canvas_pos.y + canvas_size.y * 0.5f;
		float half_height = canvas_size.y * 0.45f;

		// center line
		draw_list->AddLine( ImVec2( canvas_pos.x, center_y ), ImVec2( canvas_pos.x + canvas_size.x, center_y ), IM_COL32( 50, 50, 60, 255 ) );

		// draw waveform columns
		float col_width = canvas_size.x / k_history_width;

		for ( size_t i = 0; i < k_history_width; ++i ) {
			size_t idx;
			if ( loop_mode_ == waveform_loop_mode::scroll ) {
				idx = ( write_pos_ + i ) % k_history_width;
			} else {
				idx = i;
			}

			const auto& col = history_[ idx ];

			float x = canvas_pos.x + i * col_width;

			// calculate intensity for color mode
			float intensity = std::max( std::abs( col.max_l ), std::abs( col.max_r ) );
			ImU32 color     = get_column_color( i, intensity );

			// left channel
			float y1_l = center_y - col.max_l * half_height;
			float y2_l = center_y - col.min_l * half_height;

			if ( std::abs( col.max_l - col.min_l ) > 0.001f ) {
				draw_list->AddRectFilled( ImVec2( x, y1_l ), ImVec2( x + col_width, y2_l ), color );
			}

			// right channel (slightly different shade)
			float y1_r    = center_y - col.max_r * half_height;
			float y2_r    = center_y - col.min_r * half_height;
			ImU32 color_r = ( color & 0xFF00FFFF ) | ( ( ( color >> 8 ) & 0xFF ) * 80 / 100 ) << 8; // reduce green

			if ( std::abs( col.max_r - col.min_r ) > 0.001f ) {
				draw_list->AddRectFilled( ImVec2( x, y1_r ), ImVec2( x + col_width, y2_r ), color_r );
			}
		}

		// peak history line
		if ( show_peaks_ ) {
			draw_peak_history( draw_list, canvas_pos, canvas_size );
		}

		ImGui::Dummy( canvas_size );
	}

} // namespace pm
