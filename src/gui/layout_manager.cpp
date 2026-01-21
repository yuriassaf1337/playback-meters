// Layout manager implementation

#include "layout_manager.h"
#include "imgui.h"

namespace pm
{

	layout_manager::layout_manager( )  = default;
	layout_manager::~layout_manager( ) = default;

	void layout_manager::add_meter( std::shared_ptr< meter_panel > meter )
	{
		meters_.push_back( std::move( meter ) );
	}

	void layout_manager::remove_meter( const char* name )
	{
		meters_.erase( std::remove_if( meters_.begin( ), meters_.end( ), [ name ]( const auto& m ) { return strcmp( m->get_name( ), name ) == 0; } ),
		               meters_.end( ) );
	}

	void layout_manager::clear_meters( )
	{
		meters_.clear( );
	}

	meter_panel* layout_manager::get_meter( const char* name )
	{
		for ( auto& m : meters_ ) {
			if ( strcmp( m->get_name( ), name ) == 0 ) {
				return m.get( );
			}
		}
		return nullptr;
	}

	void layout_manager::update_all( const sample_t* samples, size_t frame_count, int channels )
	{
		for ( auto& meter : meters_ ) {
			if ( meter->is_visible( ) ) {
				meter->update( samples, frame_count, channels );
			}
		}
	}

	void layout_manager::render_all( )
	{
		switch ( mode_ ) {
		case layout_mode::horizontal_bar:
			render_horizontal_bar( );
			break;
		case layout_mode::quad:
			render_quad( );
			break;
		case layout_mode::pop_out:
			render_pop_out( );
			break;
		}
	}

	void layout_manager::render_layout_menu( )
	{
		if ( ImGui::BeginMenu( "Layout" ) ) {
			if ( ImGui::MenuItem( "Horizontal Bar", nullptr, mode_ == layout_mode::horizontal_bar ) ) {
				mode_ = layout_mode::horizontal_bar;
			}
			if ( ImGui::MenuItem( "Quad (2x2)", nullptr, mode_ == layout_mode::quad ) ) {
				mode_ = layout_mode::quad;
			}
			if ( ImGui::MenuItem( "Pop-Out Windows", nullptr, mode_ == layout_mode::pop_out ) ) {
				mode_ = layout_mode::pop_out;
			}
			ImGui::Separator( );

			if ( ImGui::BeginMenu( "Show Meters" ) ) {
				for ( auto& meter : meters_ ) {
					bool visible = meter->is_visible( );
					if ( ImGui::MenuItem( meter->get_name( ), nullptr, &visible ) ) {
						meter->set_visible( visible );
					}
				}
				ImGui::EndMenu( );
			}
			ImGui::EndMenu( );
		}
	}

	void layout_manager::render_horizontal_bar( )
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport( );

		float menu_height = ImGui::GetFrameHeight( );
		float bar_height  = 120.0f;

		ImGui::SetNextWindowPos( ImVec2( viewport->Pos.x, viewport->Pos.y + menu_height ) );
		ImGui::SetNextWindowSize( ImVec2( viewport->Size.x, bar_height ) );

		ImGui::Begin( "##HorizontalBar", nullptr,
		              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		                  ImGuiWindowFlags_NoBringToFrontOnFocus );

		// count visible meters
		int visible_count = 0;
		for ( const auto& m : meters_ ) {
			if ( m->is_visible( ) )
				visible_count++;
		}

		if ( visible_count > 0 ) {
			float meter_width = ( ImGui::GetContentRegionAvail( ).x - ( visible_count - 1 ) * 4.0f ) / visible_count;

			int i = 0;
			for ( auto& meter : meters_ ) {
				if ( !meter->is_visible( ) )
					continue;

				if ( i > 0 )
					ImGui::SameLine( 0, 4.0f );

				ImGui::BeginChild( meter->get_name( ), ImVec2( meter_width, -1 ), true );
				ImGui::Text( "%s", meter->get_name( ) );
				ImGui::Separator( );
				meter->render( );
				ImGui::EndChild( );

				i++;
			}
		}

		ImGui::End( );
	}

	void layout_manager::render_quad( )
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport( );

		float menu_height = ImGui::GetFrameHeight( );
		ImVec2 work_pos( viewport->Pos.x, viewport->Pos.y + menu_height );
		ImVec2 work_size( viewport->Size.x, viewport->Size.y - menu_height );

		// 2x2 grid
		float half_w = work_size.x / 2.0f;
		float half_h = work_size.y / 2.0f;

		ImVec2 positions[ 4 ] = { ImVec2( work_pos.x, work_pos.y ), ImVec2( work_pos.x + half_w, work_pos.y ),
			                      ImVec2( work_pos.x, work_pos.y + half_h ), ImVec2( work_pos.x + half_w, work_pos.y + half_h ) };
		ImVec2 sizes[ 4 ]     = { ImVec2( half_w, half_h ), ImVec2( half_w, half_h ), ImVec2( half_w, half_h ), ImVec2( half_w, half_h ) };

		int slot = 0;
		for ( auto& meter : meters_ ) {
			if ( !meter->is_visible( ) || slot >= 4 )
				continue;

			ImGui::SetNextWindowPos( positions[ slot ] );
			ImGui::SetNextWindowSize( sizes[ slot ] );

			ImGui::Begin( meter->get_name( ), nullptr,
			              ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
			                  ImGuiWindowFlags_NoBringToFrontOnFocus );
			meter->render( );
			ImGui::End( );

			slot++;
		}
	}

	void layout_manager::render_pop_out( )
	{
		// free-form windows - let ImGui handle positioning
		for ( auto& meter : meters_ ) {
			if ( !meter->is_visible( ) )
				continue;

			ImGui::SetNextWindowSize( meter->get_preferred_size( ), ImGuiCond_FirstUseEver );

			bool visible = true;
			ImGui::Begin( meter->get_name( ), &visible );
			if ( !visible ) {
				meter->set_visible( false );
			}
			meter->render( );
			ImGui::End( );
		}
	}

} // namespace pm
