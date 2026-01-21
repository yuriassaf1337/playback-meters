#include "application.h"
#include "../audio/audio_engine.h"
#include "../gui/layout_manager.h"

// meters
#include "../meters/loudness_meter.h"
#include "../meters/oscilloscope.h"
#include "../meters/spectrogram.h"
#include "../meters/spectrum.h"
#include "../meters/stereometer.h"
#include "../meters/vu_meter.h"
#include "../meters/waveform.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <imgui_impl_opengl3_loader.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>

namespace pm
{

	static void glfw_error_callback( int error, const char* description )
	{
		fprintf( stderr, "GLFW ERROR %d: %s\n", error, description );
	}

	application::application( ) = default;

	application::~application( )
	{
		shutdown( );
	}

	bool application::initialize( )
	{
		if ( !init_window( ) ) {
			return false;
		}

		if ( !init_audio( ) ) {
			fprintf( stderr, "WARNING: Audio engine failed to initialize\n" );
			// continue
		}

		// initialize layout manager with all meters
		layout_manager_ = std::make_unique< layout_manager >( );
		layout_manager_->add_meter( std::make_shared< oscilloscope >( ) );
		layout_manager_->add_meter( std::make_shared< spectrum >( ) );
		layout_manager_->add_meter( std::make_shared< spectrogram >( ) );
		layout_manager_->add_meter( std::make_shared< loudness_meter >( ) );
		layout_manager_->add_meter( std::make_shared< stereometer >( ) );
		layout_manager_->add_meter( std::make_shared< vu_meter >( ) );
		layout_manager_->add_meter( std::make_shared< waveform >( ) );

		// default to quad layout with first 4 meters visible
		layout_manager_->set_mode( layout_mode::quad );

		// make first 4 visible by default
		int count = 0;
		for ( auto& meter : layout_manager_->get_meters( ) ) {
			meter->set_visible( count < 4 );
			count++;
		}

		running_ = true;
		return true;
	}

	bool application::init_window( )
	{
		glfwSetErrorCallback( glfw_error_callback );

		if ( !glfwInit( ) ) {
			return false;
		}

		// OpenGL 3.0 + GLSL 130
		const char* glsl_version = "#version 130";
		glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
		glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 0 );

		window_ = glfwCreateWindow( 1280, 720, "playback-meters", nullptr, nullptr );
		if ( !window_ ) {
			glfwTerminate( );
			return false;
		}

		glfwMakeContextCurrent( window_ );
		glfwSwapInterval( 1 ); // VSync

		// setup imgui
		IMGUI_CHECKVERSION( );
		ImGui::CreateContext( );
		ImGuiIO& io = ImGui::GetIO( );
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		// try to load JetBrains Mono, fallback to default if not found
		ImFont* font = io.Fonts->AddFontFromFileTTF( "assets/fonts/JetBrainsMonoNerdFont-Regular.ttf", 15.0f );
		if ( !font ) {
			io.Fonts->AddFontDefault( );
		}

		ImGui::StyleColorsDark( );
		ImGuiStyle& style = ImGui::GetStyle( );

		style.WindowRounding    = 6.0f;
		style.FrameRounding     = 4.0f;
		style.PopupRounding     = 4.0f;
		style.ScrollbarRounding = 4.0f;
		style.GrabRounding      = 3.0f;
		style.TabRounding       = 4.0f;

		style.WindowPadding = ImVec2( 10, 10 );
		style.FramePadding  = ImVec2( 6, 4 );
		style.ItemSpacing   = ImVec2( 8, 5 );

		// CRT-inspired dark theme
		style.Colors[ ImGuiCol_WindowBg ]         = ImVec4( 0.06f, 0.06f, 0.08f, 0.98f );
		style.Colors[ ImGuiCol_TitleBg ]          = ImVec4( 0.08f, 0.08f, 0.10f, 1.0f );
		style.Colors[ ImGuiCol_TitleBgActive ]    = ImVec4( 0.10f, 0.10f, 0.14f, 1.0f );
		style.Colors[ ImGuiCol_MenuBarBg ]        = ImVec4( 0.08f, 0.08f, 0.10f, 1.0f );
		style.Colors[ ImGuiCol_Header ]           = ImVec4( 0.18f, 0.22f, 0.28f, 0.8f );
		style.Colors[ ImGuiCol_HeaderHovered ]    = ImVec4( 0.24f, 0.30f, 0.38f, 0.9f );
		style.Colors[ ImGuiCol_HeaderActive ]     = ImVec4( 0.28f, 0.35f, 0.45f, 1.0f );
		style.Colors[ ImGuiCol_Button ]           = ImVec4( 0.15f, 0.18f, 0.22f, 1.0f );
		style.Colors[ ImGuiCol_ButtonHovered ]    = ImVec4( 0.22f, 0.28f, 0.35f, 1.0f );
		style.Colors[ ImGuiCol_ButtonActive ]     = ImVec4( 0.28f, 0.35f, 0.45f, 1.0f );
		style.Colors[ ImGuiCol_FrameBg ]          = ImVec4( 0.10f, 0.10f, 0.13f, 1.0f );
		style.Colors[ ImGuiCol_FrameBgHovered ]   = ImVec4( 0.15f, 0.15f, 0.20f, 1.0f );
		style.Colors[ ImGuiCol_FrameBgActive ]    = ImVec4( 0.18f, 0.18f, 0.25f, 1.0f );
		style.Colors[ ImGuiCol_SliderGrab ]       = ImVec4( 0.40f, 0.55f, 0.70f, 1.0f );
		style.Colors[ ImGuiCol_SliderGrabActive ] = ImVec4( 0.50f, 0.65f, 0.80f, 1.0f );
		style.Colors[ ImGuiCol_CheckMark ]        = ImVec4( 0.45f, 0.70f, 0.55f, 1.0f );
		style.Colors[ ImGuiCol_Text ]             = ImVec4( 0.90f, 0.90f, 0.92f, 1.0f );
		style.Colors[ ImGuiCol_TextDisabled ]     = ImVec4( 0.50f, 0.50f, 0.55f, 1.0f );
		style.Colors[ ImGuiCol_Border ]           = ImVec4( 0.20f, 0.20f, 0.25f, 0.5f );
		style.Colors[ ImGuiCol_Separator ]        = ImVec4( 0.20f, 0.20f, 0.25f, 0.5f );

		ImGui_ImplGlfw_InitForOpenGL( window_, true );
		ImGui_ImplOpenGL3_Init( glsl_version );

		return true;
	}

	bool application::init_audio( )
	{
		audio_engine_ = std::make_unique< audio_engine >( );
		return audio_engine_->initialize( );
	}

	void application::run( )
	{
		while ( running_ && !glfwWindowShouldClose( window_ ) ) {
			main_loop( );
		}
	}

	void application::main_loop( )
	{
		glfwPollEvents( );

		// get audio samples and update meters
		if ( audio_engine_ && audio_engine_->is_capturing( ) ) {
			static std::vector< sample_t > samples( 4096 );
			size_t count = audio_engine_->get_capture( ).get_samples( samples.data( ), samples.size( ) );
			if ( count > 0 && layout_manager_ ) {
				layout_manager_->update_all( samples.data( ), count / 2, 2 ); // stereo
			}
		}

		render_frame( );
	}

	void application::render_frame( )
	{
		ImGui_ImplOpenGL3_NewFrame( );
		ImGui_ImplGlfw_NewFrame( );
		ImGui::NewFrame( );

		if ( ImGui::BeginMainMenuBar( ) ) {
			if ( ImGui::BeginMenu( "File" ) ) {
				if ( ImGui::MenuItem( "Exit" ) ) {
					request_exit( );
				}
				ImGui::EndMenu( );
			}

			if ( ImGui::BeginMenu( "Audio" ) ) {
				if ( audio_engine_ ) {
					bool capturing = audio_engine_->is_capturing( );
					if ( ImGui::MenuItem( capturing ? "Stop Capture" : "Start Capture" ) ) {
						if ( capturing ) {
							audio_engine_->stop_capture( );
						} else {
							audio_engine_->start_capture( L"", true );
						}
					}
					ImGui::Separator( );

					// device selection
					if ( ImGui::BeginMenu( "Output Devices (Loopback)" ) ) {
						auto devices = audio_engine_->get_device_enumerator( ).get_output_devices( );
						for ( const auto& dev : devices ) {
							char name[ 256 ];
							wcstombs( name, dev.name.c_str( ), sizeof( name ) );
							if ( ImGui::MenuItem( name, nullptr, dev.is_default ) ) {
								audio_engine_->start_capture( dev.id, true );
							}
						}
						ImGui::EndMenu( );
					}

					if ( ImGui::BeginMenu( "Input Devices" ) ) {
						auto devices = audio_engine_->get_device_enumerator( ).get_input_devices( );
						for ( const auto& dev : devices ) {
							char name[ 256 ];
							wcstombs( name, dev.name.c_str( ), sizeof( name ) );
							if ( ImGui::MenuItem( name, nullptr, dev.is_default ) ) {
								audio_engine_->start_capture( dev.id, false );
							}
						}
						ImGui::EndMenu( );
					}
				}
				ImGui::EndMenu( );
			}

			if ( ImGui::BeginMenu( "View" ) ) {
				if ( layout_manager_ ) {
					layout_manager_->render_layout_menu( );
				}
				ImGui::EndMenu( );
			}

			// status indicator
			if ( audio_engine_ && audio_engine_->is_capturing( ) ) {
				ImGui::Separator( );
				ImGui::TextColored( ImVec4( 0.8f, 0.3f, 0.3f, 1.0f ), "Capturing" );
			}

			ImGui::EndMainMenuBar( );
		}

		// render meters
		if ( layout_manager_ ) {
			layout_manager_->render_all( );
		}

		// render
		ImGui::Render( );
		int display_w, display_h;
		glfwGetFramebufferSize( window_, &display_w, &display_h );
		glViewport( 0, 0, display_w, display_h );

		glClearColor( 0.06f, 0.06f, 0.08f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT );

		ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData( ) );
		glfwSwapBuffers( window_ );
	}

	void application::shutdown( )
	{
		layout_manager_.reset( );

		if ( audio_engine_ ) {
			audio_engine_->shutdown( );
			audio_engine_.reset( );
		}

		if ( window_ ) {
			ImGui_ImplOpenGL3_Shutdown( );
			ImGui_ImplGlfw_Shutdown( );
			ImGui::DestroyContext( );
			glfwDestroyWindow( window_ );
			glfwTerminate( );
			window_ = nullptr;
		}

		running_ = false;
	}

} // namespace pm
