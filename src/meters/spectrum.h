#pragma once

#include "../dsp/fft_processor.h"
#include "../gui/meter_panel.h"
#include <memory>

namespace pm
{

	enum class spectrum_mode {
		bars,
		lines,
		filled
	};

	class spectrum : public meter_panel
	{
	public:
		spectrum( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		// settings
		void set_fft_size( size_t size );
		void set_mode( spectrum_mode mode )
		{
			mode_ = mode;
		}
		void set_smoothing( float smoothing )
		{
			fft_.set_smoothing( smoothing );
		}
		void set_min_db( float db )
		{
			min_db_ = db;
		}
		void set_max_db( float db )
		{
			max_db_ = db;
		}

	private:
		fft_processor fft_;
		spectrum_mode mode_ = spectrum_mode::bars;
		float min_db_       = -60.0f;
		float max_db_       = 0.0f;

		std::vector< sample_t > mono_buffer_;

		float get_band_magnitude_db( float freq_start, float freq_end ) const;
		void draw_grid( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_bars( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_lines( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_filled( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
	};

} // namespace pm
