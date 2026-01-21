#pragma once

#include "../gui/meter_panel.h"
#include <vector>

namespace pm
{

	enum class stereo_mode {
		lissajous,   // X-Y scope style
		correlation, // correlation meter
		balance      // left-right balance
	};

	class stereometer : public meter_panel
	{
	public:
		stereometer( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		void set_mode( stereo_mode mode )
		{
			mode_ = mode;
		}

	private:
		stereo_mode mode_ = stereo_mode::lissajous;

		std::vector< float > buffer_l_;
		std::vector< float > buffer_r_;
		size_t buffer_size_ = 1024;
		size_t write_pos_   = 0;

		float correlation_ = 0.0f;
		float balance_     = 0.0f;

		void draw_lissajous( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_correlation( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_balance( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
	};

} // namespace pm
