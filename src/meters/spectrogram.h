#pragma once

#include "../dsp/fft_processor.h"
#include "../gui/meter_panel.h"
#include <vector>

namespace pm
{

	class spectrogram : public meter_panel
	{
	public:
		spectrogram( );

		void update( const sample_t* samples, size_t frame_count, int channels ) override;
		void render( ) override;

		void set_fft_size( size_t size );
		void set_min_db( float db )
		{
			min_db_ = db;
		}
		void set_max_db( float db )
		{
			max_db_ = db;
		}

	private:
		static constexpr size_t k_history_width = 256;
		static constexpr int k_display_rows     = 128;

		fft_processor fft_;
		std::vector< sample_t > mono_buffer_;

		// 2D array of dB values [time][frequency]
		std::vector< std::vector< float > > history_;
		size_t write_pos_ = 0;

		float min_db_ = -60.0f;
		float max_db_ = 0.0f;

		size_t update_counter_     = 0;
		size_t updates_per_column_ = 1;

		ImU32 db_to_color( float db );
	};

} // namespace pm
