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

		// filter coefficients (approximate, assuming 48kHz sample rate)
		// low-pass at 250 Hz: alpha ~= 0.032
		// high-pass at 4000 Hz: alpha ~= 0.4
		const float lp_alpha = 0.032f;
		const float hp_alpha = 0.4f;

		// accumulators for multi-band correlation
		float low_lr = 0.0f, low_ll = 0.0f, low_rr = 0.0f;
		float mid_lr = 0.0f, mid_ll = 0.0f, mid_rr = 0.0f;
		float high_lr = 0.0f, high_ll = 0.0f, high_rr = 0.0f;

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

			// simple band splitting for multi-band correlation
			// low band: low-pass at 250 Hz
			lp_l_state_ += lp_alpha * ( l - lp_l_state_ );
			lp_r_state_ += lp_alpha * ( r - lp_r_state_ );
			float low_l = lp_l_state_;
			float low_r = lp_r_state_;

			// high band: high-pass at 4000 Hz
			hp_l_state_ += hp_alpha * ( l - hp_l_state_ );
			hp_r_state_ += hp_alpha * ( r - hp_r_state_ );
			float high_l = l - hp_l_state_;
			float high_r = r - hp_r_state_;

			// mid band: what's left after removing low and high
			float mid_l = l - low_l - high_l;
			float mid_r = r - low_r - high_r;

			// accumulate for multi-band correlation
			low_lr += low_l * low_r;
			low_ll += low_l * low_l;
			low_rr += low_r * low_r;

			mid_lr += mid_l * mid_r;
			mid_ll += mid_l * mid_l;
			mid_rr += mid_r * mid_r;

			high_lr += high_l * high_r;
			high_ll += high_l * high_l;
			high_rr += high_r * high_r;
		}

		// correlation: ranges from -1 (out of phase) to +1 (in phase)
		float denom = std::sqrt( sum_ll * sum_rr );
		if ( denom > 1e-10f ) {
			float new_corr = sum_lr / denom;
			correlation_   = correlation_ * 0.9f + new_corr * 0.1f;
		}

		// multi-band correlations
		float low_denom = std::sqrt( low_ll * low_rr );
		if ( low_denom > 1e-10f ) {
			corr_low_ = corr_low_ * 0.9f + ( low_lr / low_denom ) * 0.1f;
		}

		float mid_denom = std::sqrt( mid_ll * mid_rr );
		if ( mid_denom > 1e-10f ) {
			corr_mid_ = corr_mid_ * 0.9f + ( mid_lr / mid_denom ) * 0.1f;
		}

		float high_denom = std::sqrt( high_ll * high_rr );
		if ( high_denom > 1e-10f ) {
			corr_high_ = corr_high_ * 0.9f + ( high_lr / high_denom ) * 0.1f;
		}

		// balance: -1 = full left, +1 = full right
		float total = sum_l + sum_r;
		if ( total > 1e-10f ) {
			float new_bal = ( sum_r - sum_l ) / total;
			balance_      = balance_ * 0.9f + new_bal * 0.1f;
		}
	}

	ImU32 stereometer::get_point_color( float l, float r, float age ) const
	{
		int alpha = static_cast< int >( ( 1.0f - age * 0.8f ) * 255 );

		switch ( color_mode_ ) {
		case stereo_color_mode::static_color:
			return IM_COL32( 100, 200, 150, alpha );

		case stereo_color_mode::rgb: {
			// color based on position in stereo
			float pan = ( r - l ) / ( std::abs( l ) + std::abs( r ) + 1e-10f );

			int red   = static_cast< int >( std::max( 0.0f, pan ) * 200 + 50 );
			int green = static_cast< int >( ( 1.0f - std::abs( pan ) ) * 200 + 50 );
			int blue  = static_cast< int >( std::max( 0.0f, -pan ) * 200 + 50 );

			return IM_COL32( red, green, blue, alpha );
		}

		case stereo_color_mode::multi_band: {
			// color based on amplitude (approximating frequency content)
			// quiet = blue, medium = green, loud = red/orange
			float amp = ( std::abs( l ) + std::abs( r ) ) * 0.5f;

			if ( amp > 0.5f ) {
				return IM_COL32( 255, 100, 50, alpha ); // orange/red for loud
			} else if ( amp > 0.1f ) {
				return IM_COL32( 100, 255, 100, alpha ); // green for medium
			} else {
				return IM_COL32( 50, 150, 255, alpha ); // blue for quiet
			}
		}

		default:
			return IM_COL32( 100, 200, 150, alpha );
		}
	}

	void stereometer::render( )
	{
		ImVec2 canvas_pos  = ImGui::GetCursorScreenPos( );
		ImVec2 canvas_size = ImGui::GetContentRegionAvail( );

		if ( canvas_size.x < 50 || canvas_size.y < 50 )
			return;

		const char* display_items[] = { "Lissajous", "Scaled", "Linear" };
		int current_display         = static_cast< int >( display_mode_ );
		ImGui::SetNextItemWidth( 100.0f );
		if ( ImGui::Combo( "##display", &current_display, display_items, 3 ) ) {
			display_mode_ = static_cast< stereo_display_mode >( current_display );
		}

		ImGui::SameLine( );
		const char* color_items[] = { "Static", "RGB", "Multi-Band" };
		int current_color         = static_cast< int >( color_mode_ );
		ImGui::SetNextItemWidth( 90.0f );
		if ( ImGui::Combo( "##color", &current_color, color_items, 3 ) ) {
			color_mode_ = static_cast< stereo_color_mode >( current_color );
		}

		ImGui::SameLine( );
		const char* corr_items[] = { "Single", "Multi-Band" };
		int current_corr         = static_cast< int >( corr_mode_ );
		ImGui::SetNextItemWidth( 90.0f );
		if ( ImGui::Combo( "##corr", &current_corr, corr_items, 2 ) ) {
			corr_mode_ = static_cast< correlation_mode >( current_corr );
		}

		canvas_pos  = ImGui::GetCursorScreenPos( );
		canvas_size = ImGui::GetContentRegionAvail( );

		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		float corr_height    = ( corr_mode_ == correlation_mode::multi_band ) ? 80.0f : 50.0f;
		float display_height = canvas_size.y - corr_height - 10.0f;

		draw_list->AddRectFilled( canvas_pos, ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + display_height ), IM_COL32( 20, 20, 25, 255 ),
		                          3.0f );

		ImVec2 display_pos  = canvas_pos;
		ImVec2 display_size = ImVec2( canvas_size.x, display_height );

		switch ( display_mode_ ) {
		case stereo_display_mode::lissajous:
			draw_lissajous( draw_list, display_pos, display_size );
			break;
		case stereo_display_mode::scaled:
			draw_scaled( draw_list, display_pos, display_size );
			break;
		case stereo_display_mode::linear:
			draw_linear( draw_list, display_pos, display_size );
			break;
		}

		ImVec2 corr_pos  = ImVec2( canvas_pos.x, canvas_pos.y + display_height + 10.0f );
		ImVec2 corr_size = ImVec2( canvas_size.x, corr_height );

		if ( corr_mode_ == correlation_mode::single_band ) {
			draw_correlation( draw_list, corr_pos, corr_size );
		} else {
			draw_multiband_correlation( draw_list, corr_pos, corr_size );
		}

		ImGui::Dummy( canvas_size );
	}

	void stereometer::draw_lissajous( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		float cx    = pos.x + size.x * 0.5f;
		float cy    = pos.y + size.y * 0.5f;
		float scale = std::min( size.x, size.y ) * 0.45f;

		ImVec2 diamond[ 4 ] = {
			ImVec2( cx, cy - scale ), // top (mid)
			ImVec2( cx + scale, cy ), // right (side)
			ImVec2( cx, cy + scale ), // bottom
			ImVec2( cx - scale, cy )  // left (side negative)
		};
		draw_list->AddPolyline( diamond, 4, IM_COL32( 60, 60, 70, 255 ), ImDrawFlags_Closed, 1.0f );

		// draw crosshairs
		draw_list->AddLine( ImVec2( cx - scale, cy ), ImVec2( cx + scale, cy ), IM_COL32( 50, 50, 60, 255 ) );
		draw_list->AddLine( ImVec2( cx, cy - scale ), ImVec2( cx, cy + scale ), IM_COL32( 50, 50, 60, 255 ) );

		for ( size_t i = 0; i < buffer_size_; ++i ) {
			float l = buffer_l_[ i ];
			float r = buffer_r_[ i ];

			// rotate 45 degrees
			float x = cx + ( l - r ) * scale * 0.707f;
			float y = cy - ( l + r ) * scale * 0.707f;

			float age = static_cast< float >( ( write_pos_ + buffer_size_ - i ) % buffer_size_ ) / buffer_size_;

			draw_list->AddCircleFilled( ImVec2( x, y ), 1.5f, get_point_color( l, r, age ) );
		}

		// labels
		draw_list->AddText( ImVec2( cx - 5, pos.y + 5 ), IM_COL32( 150, 150, 150, 255 ), "M" );
		draw_list->AddText( ImVec2( pos.x + size.x - 15, cy - 5 ), IM_COL32( 150, 150, 150, 255 ), "S" );
		draw_list->AddText( ImVec2( pos.x + 5, cy - 5 ), IM_COL32( 150, 150, 150, 255 ), "-S" );
	}

	void stereometer::draw_scaled( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		float cx    = pos.x + size.x * 0.5f;
		float cy    = pos.y + size.y * 0.5f;
		float scale = std::min( size.x, size.y ) * 0.45f;

		// draw diamond with scale rings
		for ( float ring = 0.25f; ring <= 1.0f; ring += 0.25f ) {
			float ring_scale    = scale * ring;
			ImVec2 diamond[ 4 ] = { ImVec2( cx, cy - ring_scale ), ImVec2( cx + ring_scale, cy ), ImVec2( cx, cy + ring_scale ),
				                    ImVec2( cx - ring_scale, cy ) };
			ImU32 ring_color    = ( ring == 1.0f ) ? IM_COL32( 60, 60, 70, 255 ) : IM_COL32( 40, 40, 50, 200 );
			draw_list->AddPolyline( diamond, 4, ring_color, ImDrawFlags_Closed, 1.0f );
		}

		// draw samples with amplitude scaling
		for ( size_t i = 0; i < buffer_size_; ++i ) {
			float l = buffer_l_[ i ];
			float r = buffer_r_[ i ];

			float amp       = std::sqrt( l * l + r * r );
			float amp_scale = std::min( 1.0f, amp * 2.0f ); // scale up quiet signals

			float x = cx + ( l - r ) * scale * 0.707f * ( 0.3f + amp_scale * 0.7f );
			float y = cy - ( l + r ) * scale * 0.707f * ( 0.3f + amp_scale * 0.7f );

			float age = static_cast< float >( ( write_pos_ + buffer_size_ - i ) % buffer_size_ ) / buffer_size_;

			float point_size = 1.0f + amp * 2.0f;
			draw_list->AddCircleFilled( ImVec2( x, y ), point_size, get_point_color( l, r, age ) );
		}

		// labels
		draw_list->AddText( ImVec2( cx - 5, pos.y + 5 ), IM_COL32( 150, 150, 150, 255 ), "M" );
		draw_list->AddText( ImVec2( pos.x + size.x - 15, cy - 5 ), IM_COL32( 150, 150, 150, 255 ), "S" );
	}

	void stereometer::draw_linear( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		// linear L-R display (traditional oscilloscope style)
		float cx = pos.x + size.x * 0.5f;
		float cy = pos.y + size.y * 0.5f;

		// draw axes
		draw_list->AddLine( ImVec2( pos.x + 10, cy ), ImVec2( pos.x + size.x - 10, cy ), IM_COL32( 60, 60, 70, 255 ) );
		draw_list->AddLine( ImVec2( cx, pos.y + 10 ), ImVec2( cx, pos.y + size.y - 10 ), IM_COL32( 60, 60, 70, 255 ) );

		// draw box
		draw_list->AddRect( ImVec2( pos.x + 10, pos.y + 10 ), ImVec2( pos.x + size.x - 10, pos.y + size.y - 10 ), IM_COL32( 60, 60, 70, 255 ) );

		float scale_x = ( size.x - 20 ) * 0.5f;
		float scale_y = ( size.y - 20 ) * 0.5f;

		// draw samples
		for ( size_t i = 0; i < buffer_size_; ++i ) {
			float l = buffer_l_[ i ];
			float r = buffer_r_[ i ];

			float x = cx + l * scale_x;
			float y = cy - r * scale_y;

			float age = static_cast< float >( ( write_pos_ + buffer_size_ - i ) % buffer_size_ ) / buffer_size_;

			draw_list->AddCircleFilled( ImVec2( x, y ), 1.5f, get_point_color( l, r, age ) );
		}

		// labels
		draw_list->AddText( ImVec2( pos.x + size.x - 15, cy + 5 ), IM_COL32( 150, 150, 150, 255 ), "L" );
		draw_list->AddText( ImVec2( cx + 5, pos.y + 5 ), IM_COL32( 150, 150, 150, 255 ), "R" );
	}

	void stereometer::draw_correlation( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		float bar_height = 20.0f;
		float bar_y      = pos.y + ( size.y - bar_height ) * 0.5f;

		// background bar
		draw_list->AddRectFilled( ImVec2( pos.x + 10, bar_y ), ImVec2( pos.x + size.x - 10, bar_y + bar_height ), IM_COL32( 40, 40, 50, 255 ), 3.0f );

		// center marker
		float center_x = pos.x + size.x * 0.5f;
		draw_list->AddLine( ImVec2( center_x, bar_y ), ImVec2( center_x, bar_y + bar_height ), IM_COL32( 100, 100, 110, 255 ), 2.0f );

		// fill from center
		float fill_width = correlation_ * ( size.x * 0.5f - 15.0f );
		ImU32 fill_color;
		if ( correlation_ > 0.5f ) {
			fill_color = IM_COL32( 80, 200, 120, 200 );
		} else if ( correlation_ > 0.0f ) {
			fill_color = IM_COL32( 200, 200, 80, 200 );
		} else {
			fill_color = IM_COL32( 200, 80, 80, 200 );
		}

		if ( correlation_ >= 0 ) {
			draw_list->AddRectFilled( ImVec2( center_x, bar_y + 2 ), ImVec2( center_x + fill_width, bar_y + bar_height - 2 ), fill_color );
		} else {
			draw_list->AddRectFilled( ImVec2( center_x + fill_width, bar_y + 2 ), ImVec2( center_x, bar_y + bar_height - 2 ), fill_color );
		}

		// labels
		draw_list->AddText( ImVec2( pos.x + 5, bar_y + 2 ), IM_COL32( 200, 80, 80, 255 ), "-1" );
		draw_list->AddText( ImVec2( center_x - 5, bar_y - 15 ), IM_COL32( 200, 200, 200, 255 ), "0" );
		draw_list->AddText( ImVec2( pos.x + size.x - 25, bar_y + 2 ), IM_COL32( 80, 200, 120, 255 ), "+1" );

		// value
		char buf[ 32 ];
		snprintf( buf, sizeof( buf ), "%.2f", correlation_ );
		draw_list->AddText( ImVec2( center_x - 15, bar_y + bar_height + 2 ), IM_COL32( 200, 200, 200, 255 ), buf );
	}

	void stereometer::draw_multiband_correlation( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		float bar_height = 15.0f;
		float spacing    = 5.0f;
		float bar_width  = size.x - 80.0f;

		const char* labels[] = { "Low", "Mid", "High", "All" };
		float* values[]      = { &corr_low_, &corr_mid_, &corr_high_, &correlation_ };
		ImU32 band_colors[]  = { IM_COL32( 200, 100, 100, 200 ), IM_COL32( 100, 200, 100, 200 ), IM_COL32( 100, 100, 200, 200 ),
			                     IM_COL32( 200, 200, 200, 200 ) };

		for ( int b = 0; b < 4; ++b ) {
			float bar_y = pos.y + b * ( bar_height + spacing );

			// label
			draw_list->AddText( ImVec2( pos.x + 5, bar_y + 1 ), IM_COL32( 150, 150, 150, 255 ), labels[ b ] );

			// background bar
			float bar_x = pos.x + 40.0f;
			draw_list->AddRectFilled( ImVec2( bar_x, bar_y ), ImVec2( bar_x + bar_width, bar_y + bar_height ), IM_COL32( 40, 40, 50, 255 ), 2.0f );

			// center marker
			float center_x = bar_x + bar_width * 0.5f;
			draw_list->AddLine( ImVec2( center_x, bar_y ), ImVec2( center_x, bar_y + bar_height ), IM_COL32( 80, 80, 90, 255 ), 1.0f );

			// fill
			float corr   = *values[ b ];
			float fill_w = corr * ( bar_width * 0.5f - 5.0f );

			ImU32 fill_color = band_colors[ b ];
			if ( corr < 0 ) {
				fill_color = IM_COL32( 200, 80, 80, 200 );
			}

			if ( corr >= 0 ) {
				draw_list->AddRectFilled( ImVec2( center_x, bar_y + 2 ), ImVec2( center_x + fill_w, bar_y + bar_height - 2 ), fill_color );
			} else {
				draw_list->AddRectFilled( ImVec2( center_x + fill_w, bar_y + 2 ), ImVec2( center_x, bar_y + bar_height - 2 ), fill_color );
			}

			// value
			char buf[ 16 ];
			snprintf( buf, sizeof( buf ), "%.2f", corr );
			draw_list->AddText( ImVec2( bar_x + bar_width + 5, bar_y + 1 ), IM_COL32( 180, 180, 180, 255 ), buf );
		}
	}

} // namespace pm
