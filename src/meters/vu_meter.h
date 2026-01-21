#pragma once

#include "../gui/meter_panel.h"

namespace pm
{

	class vu_meter : public meter_panel
	{
	public:
		vu_meter( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		void set_calibration( float db )
		{
			calibration_db_ = db;
		}

	private:
		float vu_l_   = -40.0f;
		float vu_r_   = -40.0f;
		float peak_l_ = -40.0f;
		float peak_r_ = -40.0f;

		float calibration_db_ = 0.0f; // 0 VU = ?

		// VU ballistics (300ms integration time)
		float integration_coeff_ = 0.0f;

		void draw_vu_arc( ImDrawList* draw_list, ImVec2 center, float radius, float value_vu, float peak_vu, bool is_left );
	};

} // namespace pm
