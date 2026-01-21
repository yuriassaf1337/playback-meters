#pragma once

#include "meter_panel.h"
#include <memory>
#include <vector>

namespace pm
{

	enum class layout_mode {
		horizontal_bar, // toolbar-like strip for compact monitoring
		quad,           // 2x2 grid layout
		pop_out         // freeform arrangement of individual meters
	};

	class layout_manager
	{
	public:
		layout_manager( );
		~layout_manager( );

		// layout mode
		void set_mode( layout_mode mode )
		{
			mode_ = mode;
		}
		layout_mode get_mode( ) const
		{
			return mode_;
		}

		// meter management
		void add_meter( std::shared_ptr< meter_panel > meter );
		void remove_meter( const char* name );
		void clear_meters( );

		// get meter by name
		meter_panel* get_meter( const char* name );

		// update all meters with audio data
		void update_all( const sample_t* samples, size_t frame_count, int channels );

		// render all visible meters according to current layout
		void render_all( );

		// render layout selection menu
		void render_layout_menu( );

		// get all meters (for visibility toggles, etc.)
		const std::vector< std::shared_ptr< meter_panel > >& get_meters( ) const
		{
			return meters_;
		}

	private:
		layout_mode mode_ = layout_mode::quad;
		std::vector< std::shared_ptr< meter_panel > > meters_;

		void render_horizontal_bar( );
		void render_quad( );
		void render_pop_out( );
	};

} // namespace pm
