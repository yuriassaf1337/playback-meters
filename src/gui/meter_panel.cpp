#include "meter_panel.h"

namespace pm
{

	void meter_panel::begin_panel( )
	{
		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8, 8 ) );
		ImGui::Begin( name_.c_str( ), &visible_, ImGuiWindowFlags_NoScrollbar );
	}

	void meter_panel::end_panel( )
	{
		ImGui::End( );
		ImGui::PopStyleVar( );
	}

	ImU32 meter_panel::color_from_db( float db, float min_db, float max_db )
	{
		// normalize dB to 0-1 range
		float t = ( db - min_db ) / ( max_db - min_db );
		t       = t < 0.0f ? 0.0f : ( t > 1.0f ? 1.0f : t );

		// color gradient: blue (quiet) -> green -> yellow -> red (loud)
		if ( t < 0.5f ) {
			// blue to green
			float local_t = t * 2.0f;
			return lerp_color( IM_COL32( 50, 100, 200, 255 ), IM_COL32( 50, 200, 100, 255 ), local_t );
		} else if ( t < 0.75f ) {
			// green to yellow
			float local_t = ( t - 0.5f ) * 4.0f;
			return lerp_color( IM_COL32( 50, 200, 100, 255 ), IM_COL32( 230, 200, 50, 255 ), local_t );
		} else {
			// yellow to red
			float local_t = ( t - 0.75f ) * 4.0f;
			return lerp_color( IM_COL32( 230, 200, 50, 255 ), IM_COL32( 230, 50, 50, 255 ), local_t );
		}
	}

	ImU32 meter_panel::lerp_color( ImU32 a, ImU32 b, float t )
	{
		int ra = ( a >> IM_COL32_R_SHIFT ) & 0xFF;
		int ga = ( a >> IM_COL32_G_SHIFT ) & 0xFF;
		int ba = ( a >> IM_COL32_B_SHIFT ) & 0xFF;
		int aa = ( a >> IM_COL32_A_SHIFT ) & 0xFF;

		int rb = ( b >> IM_COL32_R_SHIFT ) & 0xFF;
		int gb = ( b >> IM_COL32_G_SHIFT ) & 0xFF;
		int bb = ( b >> IM_COL32_B_SHIFT ) & 0xFF;
		int ab = ( b >> IM_COL32_A_SHIFT ) & 0xFF;

		int r  = static_cast< int >( ra + ( rb - ra ) * t );
		int g  = static_cast< int >( ga + ( gb - ga ) * t );
		int bl = static_cast< int >( ba + ( bb - ba ) * t );
		int al = static_cast< int >( aa + ( ab - aa ) * t );

		return IM_COL32( r, g, bl, al );
	}

} // namespace pm
