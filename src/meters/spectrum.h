#pragma once

#include "../dsp/fft_processor.h"
#include "../gui/meter_panel.h"
#include <memory>
#include <string>

namespace pm
{

	enum class spectrum_display_mode {
		fft,        // fft line only
		color_bars, // gradient color bars only
		both        // bars + fft line overlay
	};

	enum class spectrum_scale {
		linear,
		logarithmic,
		mel
	};

	enum class spectrum_channel {
		left,
		right,
		mid,
		side
	};

	struct peak_info {
		float frequency = 0.0f;
		float db        = -100.0f;
		float x         = 0.0f; // screen position
		float y         = 0.0f;
	};

	class spectrum : public meter_panel
	{
	public:
		spectrum( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		// settings
		void set_fft_size( size_t size );
		void set_display_mode( spectrum_display_mode mode )
		{
			display_mode_ = mode;
		}
		void set_scale( spectrum_scale scale )
		{
			scale_ = scale;
		}
		void set_channel( spectrum_channel channel )
		{
			channel_ = channel;
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
		void set_show_peak_info( bool show )
		{
			show_peak_info_ = show;
		}

	private:
		fft_processor fft_;
		spectrum_display_mode display_mode_ = spectrum_display_mode::both;
		spectrum_scale scale_               = spectrum_scale::logarithmic;
		spectrum_channel channel_           = spectrum_channel::left;
		float min_db_                       = -60.0f;
		float max_db_                       = 0.0f;
		bool show_peak_info_                = true;

		std::vector< sample_t > left_buffer_;
		std::vector< sample_t > right_buffer_;
		peak_info peak_;

		// scale conversion
		float position_to_freq( float pos ) const;
		float freq_to_position( float freq ) const;

		// rendering
		float get_band_magnitude_db( float freq_start, float freq_end ) const;
		void find_peak( ImVec2 pos, ImVec2 size );
		void draw_grid( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_color_bars( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_fft_line( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );
		void draw_peak_tooltip( ImDrawList* draw_list, ImVec2 pos, ImVec2 size );

		// color gradient for bars
		ImU32 get_bar_color( float t ) const;
	};

} // namespace pm
