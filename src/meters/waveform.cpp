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
			acc_samples_++;

			// when we have enough samples for one column
			size_t threshold = static_cast< size_t >( samples_per_column_ / scroll_speed_ );
			if ( acc_samples_ >= threshold ) {
				history_[ write_pos_ ].min_l = acc_min_l_;
				history_[ write_pos_ ].max_l = acc_max_l_;
				history_[ write_pos_ ].min_r = acc_min_r_;
				history_[ write_pos_ ].max_r = acc_max_r_;

				write_pos_ = ( write_pos_ + 1 ) % k_history_width;

				acc_min_l_ = acc_max_l_ = 0.0f;
				acc_min_r_ = acc_max_r_ = 0.0f;
				acc_samples_            = 0;
			}
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
		draw_list->AddRectFilled( canvas_pos, ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ), IM_COL32( 20, 20, 25, 255 ) );

		float center_y    = canvas_pos.y + canvas_size.y * 0.5f;
		float half_height = canvas_size.y * 0.45f;

		// center line
		draw_list->AddLine( ImVec2( canvas_pos.x, center_y ), ImVec2( canvas_pos.x + canvas_size.x, center_y ), IM_COL32( 60, 60, 70, 255 ) );

		// draw waveform columns
		float col_width = canvas_size.x / k_history_width;

		for ( size_t i = 0; i < k_history_width; ++i ) {
			size_t idx      = ( write_pos_ + i ) % k_history_width;
			const auto& col = history_[ idx ];

			float x = canvas_pos.x + i * col_width;

			// left channel (cyan) - top half
			float y1_l = center_y - col.max_l * half_height;
			float y2_l = center_y - col.min_l * half_height;

			if ( std::abs( col.max_l - col.min_l ) > 0.001f ) {
				draw_list->AddRectFilled( ImVec2( x, y1_l ), ImVec2( x + col_width, y2_l ), IM_COL32( 80, 180, 200, 180 ) );
			}

			// right channel (magenta) - blend with left
			float y1_r = center_y - col.max_r * half_height;
			float y2_r = center_y - col.min_r * half_height;

			if ( std::abs( col.max_r - col.min_r ) > 0.001f ) {
				draw_list->AddRectFilled( ImVec2( x, y1_r ), ImVec2( x + col_width, y2_r ), IM_COL32( 200, 80, 180, 120 ) );
			}
		}

		// peak markers
		if ( show_peaks_ ) {
			float max_peak = 0.0f;
			for ( const auto& col : history_ ) {
				max_peak = std::max( max_peak, std::abs( col.max_l ) );
				max_peak = std::max( max_peak, std::abs( col.min_l ) );
				max_peak = std::max( max_peak, std::abs( col.max_r ) );
				max_peak = std::max( max_peak, std::abs( col.min_r ) );
			}

			if ( max_peak > 0.01f ) {
				float peak_y_top = center_y - max_peak * half_height;
				float peak_y_bot = center_y + max_peak * half_height;

				draw_list->AddLine( ImVec2( canvas_pos.x, peak_y_top ), ImVec2( canvas_pos.x + canvas_size.x, peak_y_top ),
				                    IM_COL32( 100, 100, 120, 100 ) );
				draw_list->AddLine( ImVec2( canvas_pos.x, peak_y_bot ), ImVec2( canvas_pos.x + canvas_size.x, peak_y_bot ),
				                    IM_COL32( 100, 100, 120, 100 ) );
			}
		}

		ImGui::Dummy( canvas_size );
	}

} // namespace pm
