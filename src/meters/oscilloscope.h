#pragma once

#include "../gui/meter_panel.h"
#include <vector>

namespace pm
{

	class oscilloscope : public meter_panel
	{
	public:
		oscilloscope( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		void set_zoom( float zoom )
		{
			zoom_ = zoom;
		}
		void set_show_grid( bool show )
		{
			show_grid_ = show;
		}
		void set_channel_mode( int mode )
		{
			channel_mode_ = mode;
		} // 0=both, 1=left, 2=right, 3=mid, 4=side

	private:
		std::vector< float > buffer_l_;
		std::vector< float > buffer_r_;
		size_t buffer_size_ = 2048;
		size_t write_pos_   = 0;

		float zoom_       = 1.0f;
		bool show_grid_   = true;
		int channel_mode_ = 0;

		void draw_waveform( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_grid( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
	};

} // namespace pm
