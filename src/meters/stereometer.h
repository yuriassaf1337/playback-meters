#pragma once

#include "../gui/meter_panel.h"
#include <vector>

namespace pm
{

	enum class stereo_display_mode {
		lissajous, // rotated diamond X-Y scope
		scaled,    // scaled diamond with amplitude emphasis
		linear     // linear L-R display
	};

	enum class stereo_color_mode {
		static_color, // single color
		rgb,          // color based on position (RGB gradient)
		multi_band    // colored by frequency band
	};

	enum class correlation_mode {
		single_band, // overall correlation
		multi_band   // low/mid/high correlation
	};

	class stereometer : public meter_panel
	{
	public:
		stereometer( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		void set_display_mode( stereo_display_mode mode )
		{
			display_mode_ = mode;
		}
		void set_color_mode( stereo_color_mode mode )
		{
			color_mode_ = mode;
		}
		void set_correlation_mode( correlation_mode mode )
		{
			corr_mode_ = mode;
		}

	private:
		stereo_display_mode display_mode_ = stereo_display_mode::lissajous;
		stereo_color_mode color_mode_     = stereo_color_mode::rgb;
		correlation_mode corr_mode_       = correlation_mode::single_band;

		std::vector< float > buffer_l_;
		std::vector< float > buffer_r_;
		size_t buffer_size_ = 1024;
		size_t write_pos_   = 0;

		// correlation values
		float correlation_ = 0.0f;
		float balance_     = 0.0f;

		float corr_low_  = 0.0f; // 20-250 Hz
		float corr_mid_  = 0.0f; // 250-4000 Hz
		float corr_high_ = 0.0f; // 4000-20000 Hz

		float low_l_acc_ = 0.0f, low_r_acc_ = 0.0f;
		float mid_l_acc_ = 0.0f, mid_r_acc_ = 0.0f;
		float high_l_acc_ = 0.0f, high_r_acc_ = 0.0f;

		float lp_l_state_ = 0.0f, lp_r_state_ = 0.0f;
		float hp_l_state_ = 0.0f, hp_r_state_ = 0.0f;
		float bp_l_state1_ = 0.0f, bp_r_state1_ = 0.0f;
		float bp_l_state2_ = 0.0f, bp_r_state2_ = 0.0f;

		void draw_lissajous( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_scaled( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_linear( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_correlation( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_multiband_correlation( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );

		ImU32 get_point_color( float l, float r, float age ) const;
	};

} // namespace pm
