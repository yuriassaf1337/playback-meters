#pragma once

#include "../gui/meter_panel.h"
#include <vector>

namespace pm
{

	enum class waveform_color_mode {
		static_color, // single color
		multi_band,   // frequency-based coloring
		color_map     // intensity-based colormap
	};

	enum class waveform_loop_mode {
		scroll,     // scrolling waveform
		static_loop // static loop buffer
	};

	class waveform : public meter_panel
	{
	public:
		waveform( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		void set_scroll_speed( float speed )
		{
			scroll_speed_ = speed;
		}
		void set_show_peaks( bool show )
		{
			show_peaks_ = show;
		}
		void set_color_mode( waveform_color_mode mode )
		{
			color_mode_ = mode;
		}
		void set_loop_mode( waveform_loop_mode mode )
		{
			loop_mode_ = mode;
		}

	private:
		static constexpr size_t k_history_width = 512;

		struct waveform_column {
			float min_l, max_l;
			float min_r, max_r;
			float rms_l, rms_r; // for peak history
		};

		std::vector< waveform_column > history_;
		size_t write_pos_ = 0;

		waveform_color_mode color_mode_ = waveform_color_mode::static_color;
		waveform_loop_mode loop_mode_   = waveform_loop_mode::scroll;
		float scroll_speed_             = 1.0f;
		bool show_peaks_                = true;

		// accumulator for current column
		float acc_min_l_ = 0.0f, acc_max_l_ = 0.0f;
		float acc_min_r_ = 0.0f, acc_max_r_ = 0.0f;
		float acc_sum_l_ = 0.0f, acc_sum_r_ = 0.0f;
		size_t acc_samples_        = 0;
		size_t samples_per_column_ = 256;

		// peak history
		float peak_history_[ k_history_width ] = { 0.0f };

		ImU32 get_column_color( size_t idx, float intensity ) const;
		void draw_peak_history( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
	};

} // namespace pm
