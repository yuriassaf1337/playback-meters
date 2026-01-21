#include "oscilloscope.h"
#include <algorithm>
#include <cmath>

namespace pm
{

	oscilloscope::oscilloscope( ) : meter_panel( "Oscilloscope" )
	{
		buffer_l_.resize( buffer_size_, 0.0f );
		buffer_r_.resize( buffer_size_, 0.0f );
	}

	void oscilloscope::update( const sample_t* samples, size_t frame_count, int channels )
	{
		for ( size_t i = 0; i < frame_count; ++i ) {
			buffer_l_[ write_pos_ ] = ( channels >= 1 ) ? samples[ i * channels ] : 0.0f;
			buffer_r_[ write_pos_ ] = ( channels >= 2 ) ? samples[ i * channels + 1 ] : buffer_l_[ write_pos_ ];
			write_pos_              = ( write_pos_ + 1 ) % buffer_size_;
		}
	}

	void oscilloscope::render( )
	{
		ImVec2 canvas_pos  = ImGui::GetCursorScreenPos( );
		ImVec2 canvas_size = ImGui::GetContentRegionAvail( );

		if ( canvas_size.x < 50 || canvas_size.y < 50 )
			return;

		ImDrawList* draw_list = ImGui::GetWindowDrawList( );

		// background
		draw_list->AddRectFilled( canvas_pos, ImVec2( canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y ), IM_COL32( 20, 20, 25, 255 ) );

		if ( show_grid_ ) {
			draw_grid( draw_list, canvas_pos, canvas_size );
		}

		draw_waveform( draw_list, canvas_pos, canvas_size );

		// dummy to capture the space
		ImGui::Dummy( canvas_size );
	}

	void oscilloscope::draw_grid( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		ImU32 grid_color = IM_COL32( 60, 60, 70, 255 );

		// center line
		float center_y = pos.y + size.y * 0.5f;
		draw_list->AddLine( ImVec2( pos.x, center_y ), ImVec2( pos.x + size.x, center_y ), grid_color );

		// horizontal divisions
		for ( int i = 1; i <= 4; ++i ) {
			float offset = size.y * 0.25f * i * 0.5f;
			draw_list->AddLine( ImVec2( pos.x, center_y - offset ), ImVec2( pos.x + size.x, center_y - offset ), IM_COL32( 40, 40, 50, 255 ) );
			draw_list->AddLine( ImVec2( pos.x, center_y + offset ), ImVec2( pos.x + size.x, center_y + offset ), IM_COL32( 40, 40, 50, 255 ) );
		}

		// vertical divisions
		for ( int i = 1; i < 8; ++i ) {
			float x = pos.x + size.x * i / 8.0f;
			draw_list->AddLine( ImVec2( x, pos.y ), ImVec2( x, pos.y + size.y ), IM_COL32( 40, 40, 50, 255 ) );
		}
	}

	void oscilloscope::draw_waveform( ImDrawList* draw_list, ImVec2 pos, ImVec2 size )
	{
		size_t display_samples = static_cast< size_t >( buffer_size_ / zoom_ );
		if ( display_samples < 2 )
			display_samples = 2;
		if ( display_samples > buffer_size_ )
			display_samples = buffer_size_;

		float center_y  = pos.y + size.y * 0.5f;
		float amplitude = size.y * 0.45f;

		// draw left channel (cyan)
		if ( channel_mode_ == 0 || channel_mode_ == 1 ) {
			std::vector< ImVec2 > points_l;
			points_l.reserve( display_samples );

			for ( size_t i = 0; i < display_samples; ++i ) {
				size_t idx = ( write_pos_ + buffer_size_ - display_samples + i ) % buffer_size_;
				float x    = pos.x + ( i / ( float )( display_samples - 1 ) ) * size.x;
				float y    = center_y - buffer_l_[ idx ] * amplitude;
				points_l.push_back( ImVec2( x, y ) );
			}

			if ( points_l.size( ) >= 2 ) {
				draw_list->AddPolyline( points_l.data( ), static_cast< int >( points_l.size( ) ), IM_COL32( 80, 200, 220, 255 ), ImDrawFlags_None,
				                        1.5f );
			}
		}

		// draw right channel (magenta)
		if ( channel_mode_ == 0 || channel_mode_ == 2 ) {
			std::vector< ImVec2 > points_r;
			points_r.reserve( display_samples );

			for ( size_t i = 0; i < display_samples; ++i ) {
				size_t idx = ( write_pos_ + buffer_size_ - display_samples + i ) % buffer_size_;
				float x    = pos.x + ( i / ( float )( display_samples - 1 ) ) * size.x;
				float y    = center_y - buffer_r_[ idx ] * amplitude;
				points_r.push_back( ImVec2( x, y ) );
			}

			if ( points_r.size( ) >= 2 ) {
				draw_list->AddPolyline( points_r.data( ), static_cast< int >( points_r.size( ) ), IM_COL32( 220, 80, 180, 255 ), ImDrawFlags_None,
				                        1.5f );
			}
		}
	}

} // namespace pm
