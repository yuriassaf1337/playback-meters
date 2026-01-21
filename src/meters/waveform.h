#pragma once

#include "../gui/meter_panel.h"
#include <vector>

namespace pm
{

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

	private:
		static constexpr size_t k_history_width = 512;

		struct waveform_column {
			float min_l, max_l;
			float min_r, max_r;
		};

		std::vector< waveform_column > history_;
		size_t write_pos_ = 0;

		float scroll_speed_ = 1.0f;
		bool show_peaks_    = true;

		// accumulator for current column
		float acc_min_l_ = 0.0f, acc_max_l_ = 0.0f;
		float acc_min_r_ = 0.0f, acc_max_r_ = 0.0f;
		size_t acc_samples_        = 0;
		size_t samples_per_column_ = 256;
	};

} // namespace pm
