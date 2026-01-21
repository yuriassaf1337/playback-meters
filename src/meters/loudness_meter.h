#pragma once

#include "../dsp/loudness.h"
#include "../gui/meter_panel.h"

namespace pm
{

	enum class loudness_mode {
		lufs_momentary, // 0.4s window
		lufs_short,     // 3.0s window
		rms_fast,       // 0.3s window
		rms_slow        // 1.0s window
	};

	class loudness_meter : public meter_panel
	{
	public:
		loudness_meter( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		void set_mode( loudness_mode mode )
		{
			mode_ = mode;
		}

	private:
		lufs_meter lufs_;
		loudness_mode mode_ = loudness_mode::lufs_momentary;

		float peak_l_ = -100.0f;
		float peak_r_ = -100.0f;
		float rms_l_  = -100.0f;
		float rms_r_  = -100.0f;

		// rms buffers for different windows
		float rms_fast_ = -100.0f; // 0.3s
		float rms_slow_ = -100.0f; // 1.0s

		// peak hold
		float peak_hold_     = -100.0f;
		int peak_hold_timer_ = 0;

		// rms accumulators
		float rms_fast_sum_    = 0.0f;
		size_t rms_fast_count_ = 0;
		float rms_slow_sum_    = 0.0f;
		size_t rms_slow_count_ = 0;

		void draw_vertical_meter( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		float get_display_value( ) const;
		const char* get_mode_label( ) const;
	};

} // namespace pm
