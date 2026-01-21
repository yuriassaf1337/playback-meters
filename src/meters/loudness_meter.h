#pragma once

// peak, RMS, LUFS display

#include "../dsp/loudness.h"
#include "../gui/meter_panel.h"

namespace pm
{

	class loudness_meter : public meter_panel
	{
	public:
		loudness_meter( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

	private:
		lufs_meter lufs_;

		float peak_l_ = -100.0f;
		float peak_r_ = -100.0f;
		float rms_l_  = -100.0f;
		float rms_r_  = -100.0f;

		float peak_hold_l_   = -100.0f;
		float peak_hold_r_   = -100.0f;
		int peak_hold_timer_ = 0;

		void draw_meter_bar( ImDrawList* draw_list, ImVec2 pos, ImVec2 size, float value_db, float peak_db, const char* label );
	};

} // namespace pm
