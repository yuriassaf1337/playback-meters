#pragma once

#include "../common/types.h"
#include "imgui.h"
#include <string>

namespace pm
{

	// fw
	class audio_engine;

	// meter panel base class - all meters inherit from this
	class meter_panel
	{
	public:
		meter_panel( const char* name ) : name_( name ) { }
		virtual ~meter_panel( ) = default;

		// update meter with new audio samples
		virtual void update( const sample_t* samples, size_t frame_count, int channels ) = 0;

		// render the meter visualization
		virtual void render( ) = 0;

		// get meter name (for window title, etc.)
		virtual const char* get_name( ) const
		{
			return name_.c_str( );
		}

		bool is_visible( ) const
		{
			return visible_;
		}
		void set_visible( bool visible )
		{
			visible_ = visible;
		}
		void toggle_visible( )
		{
			visible_ = !visible_;
		}

		virtual ImVec2 get_min_size( ) const
		{
			return ImVec2( 200, 150 );
		}
		virtual ImVec2 get_preferred_size( ) const
		{
			return ImVec2( 400, 300 );
		}

	protected:
		std::string name_;
		bool visible_ = true;

		// common rendering helpers
		void begin_panel( );
		void end_panel( );

		// color utilities for visualization
		static ImU32 color_from_db( float db, float min_db = -60.0f, float max_db = 0.0f );
		static ImU32 lerp_color( ImU32 a, ImU32 b, float t );
	};

} // namespace pm
