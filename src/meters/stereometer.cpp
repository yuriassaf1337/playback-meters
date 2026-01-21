#include "stereometer.h"
#include <algorithm>
#include <cmath>

namespace pm
{

	stereometer::stereometer( ) : meter_panel( "Stereometer" )
	{
		buffer_l_.resize( buffer_size_, 0.0f );
		buffer_r_.resize( buffer_size_, 0.0f );
	}

	void stereometer::update( const sample_t* samples, size_t frame_count, int channels )
	{
		if ( channels < 2 )
			return;

		float sum_lr = 0.0f, sum_ll = 0.0f, sum_rr = 0.0f;
		float sum_l = 0.0f, sum_r = 0.0f;

		for ( size_t i = 0; i < frame_count; ++i ) {
			float l = samples[ i * channels ];
			float r = samples[ i * channels + 1 ];

			buffer_l_[ write_pos_ ] = l;
			buffer_r_[ write_pos_ ] = r;
			write_pos_              = ( write_pos_ + 1 ) % buffer_size_;

			sum_lr += l * r;
			sum_ll += l * l;
			sum_rr += r * r;
			sum_l += std::abs( l );
			sum_r += std::abs( r );
		}

		// correlation: ranges from -1 (out of phase) to +1 (in phase)
		float denom = std::sqrt( sum_ll * sum_rr );
		if ( denom > 1e-10f ) {
			float new_corr = sum_lr / denom;
			correlation_   = correlation_ * 0.9f + new_corr * 0.1f; // smooth
		}

		// balance: -1 = full left, +1 = full right
		float total = sum_l + sum_r;
		if ( total > 1e-10f ) {
			float new_bal = ( sum_r - sum_l ) / total;
			balance_      = balance_ * 0.9f + new_bal * 0.1f;
		}
	}

	void stereometer::render( )
	{
		ImVec2 canvas_pos  = ImGui::GetCursorScreenPos( );
		ImVec2 canvas_size = ImGui::GetContentRegionAvail( );

		if ( canvas_size.x < 50 || canvas_size.y < 50 )
			return;

		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		// background
		draw_list->AddRectFilled( canvas_pos, ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ), IM_COL32( 20, 20, 25, 255 ) );

		switch ( mode_ ) {
		case stereo_mode::lissajous:
			draw_lissajous( draw_list, canvas_pos, canvas_size );
			break;
		case stereo_mode::correlation:
			draw_correlation( draw_list, canvas_pos, canvas_size );
			break;
		case stereo_mode::balance:
			draw_balance( draw_list, canvas_pos, canvas_size );
			break;
		}

		ImGui::Dummy( canvas_size );
	}

	void stereometer::draw_lissajous( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		float cx    = pos.x + size.x * 0.5f;
		float cy    = pos.y + size.y * 0.5f;
		float scale = std::min( size.x, size.y ) * 0.45f;

		// draw crosshairs
		draw_list->AddLine( ImVec2( cx - scale, cy ), ImVec2( cx + scale, cy ), IM_COL32( 60, 60, 70, 255 ) );
		draw_list->AddLine( ImVec2( cx, cy - scale ), ImVec2( cx, cy + scale ), IM_COL32( 60, 60, 70, 255 ) );

		// draw diagonal lines (L+R and L-R axes)
		draw_list->AddLine( ImVec2( cx - scale, cy - scale ), ImVec2( cx + scale, cy + scale ), IM_COL32( 40, 40, 50, 255 ) );
		draw_list->AddLine( ImVec2( cx - scale, cy + scale ), ImVec2( cx + scale, cy - scale ), IM_COL32( 40, 40, 50, 255 ) );

		// draw samples as points
		for ( size_t i = 0; i < buffer_size_; ++i ) {
			float l = buffer_l_[ i ];
			float r = buffer_r_[ i ];

			// rotate 45 degrees for M/S display
			float x = cx + ( l - r ) * scale * 0.707f;
			float y = cy - ( l + r ) * scale * 0.5f;

			// fade based on age
			float age = static_cast< float >( ( write_pos_ + buffer_size_ - i ) % buffer_size_ ) / buffer_size_;
			int alpha = static_cast< int >( ( 1.0f - age * 0.8f ) * 255 );

			draw_list->AddCircleFilled( ImVec2( x, y ), 1.5f, IM_COL32( 100, 200, 150, alpha ) );
		}

		// labels
		draw_list->AddText( ImVec2( pos.x + 5, pos.y + 5 ), IM_COL32( 150, 150, 150, 255 ), "L" );
		draw_list->AddText( ImVec2( pos.x + size.x - 15, pos.y + 5 ), IM_COL32( 150, 150, 150, 255 ), "R" );
	}

	void stereometer::draw_correlation( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		float bar_height = 30.0f;
		float bar_y      = pos.y + ( size.y - bar_height ) * 0.5f;

		// background bar
		draw_list->AddRectFilled( ImVec2( pos.x + 10, bar_y ), ImVec2( pos.x + size.x - 10, bar_y + bar_height ), IM_COL32( 40, 40, 50, 255 ) );

		// center marker
		float center_x = pos.x + size.x * 0.5f;
		draw_list->AddLine( ImVec2( center_x, bar_y ), ImVec2( center_x, bar_y + bar_height ), IM_COL32( 100, 100, 110, 255 ), 2.0f );

		// correlation indicator
		float indicator_x = center_x + correlation_ * ( size.x * 0.5f - 15.0f );

		ImU32 color;
		if ( correlation_ > 0.5f ) {
			color = IM_COL32( 80, 200, 120, 255 ); // good (in phase)
		} else if ( correlation_ > 0.0f ) {
			color = IM_COL32( 200, 200, 80, 255 ); // OK
		} else {
			color = IM_COL32( 200, 80, 80, 255 ); // bad (out of phase)
		}

		draw_list->AddCircleFilled( ImVec2( indicator_x, bar_y + bar_height * 0.5f ), 8.0f, color );

		// labels
		draw_list->AddText( ImVec2( pos.x + 15, bar_y + 7 ), IM_COL32( 200, 80, 80, 255 ), "-1" );
		draw_list->AddText( ImVec2( center_x - 5, bar_y - 20 ), IM_COL32( 200, 200, 200, 255 ), "0" );
		draw_list->AddText( ImVec2( pos.x + size.x - 30, bar_y + 7 ), IM_COL32( 80, 200, 120, 255 ), "+1" );

		// value text
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), "Correlation: %.2f", correlation_ );
		draw_list->AddText( ImVec2( pos.x + 10, pos.y + size.y - 25 ), IM_COL32( 200, 200, 200, 255 ), buf );
	}

	void stereometer::draw_balance( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		float bar_height = 30.0f;
		float bar_y      = pos.y + ( size.y - bar_height ) * 0.5f;

		// background bar
		draw_list->AddRectFilled( ImVec2( pos.x + 10, bar_y ), ImVec2( pos.x + size.x - 10, bar_y + bar_height ), IM_COL32( 40, 40, 50, 255 ) );

		// center marker
		float center_x = pos.x + size.x * 0.5f;
		draw_list->AddLine( ImVec2( center_x, bar_y - 5 ), ImVec2( center_x, bar_y + bar_height + 5 ), IM_COL32( 100, 100, 110, 255 ), 2.0f );

		// fill from center to balance position
		float balance_x = center_x + balance_ * ( size.x * 0.5f - 15.0f );

		if ( balance_ < 0 ) {
			draw_list->AddRectFilled( ImVec2( balance_x, bar_y + 2 ), ImVec2( center_x, bar_y + bar_height - 2 ), IM_COL32( 80, 200, 200, 200 ) );
		} else {
			draw_list->AddRectFilled( ImVec2( center_x, bar_y + 2 ), ImVec2( balance_x, bar_y + bar_height - 2 ), IM_COL32( 200, 80, 200, 200 ) );
		}

		// labels
		draw_list->AddText( ImVec2( pos.x + 15, bar_y + 7 ), IM_COL32( 80, 200, 200, 255 ), "L" );
		draw_list->AddText( ImVec2( pos.x + size.x - 25, bar_y + 7 ), IM_COL32( 200, 80, 200, 255 ), "R" );
	}

} // namespace pm
