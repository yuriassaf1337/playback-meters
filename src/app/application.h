#pragma once

#include <memory>

struct GLFWwindow;

namespace pm
{

	// fw
	class audio_engine;

	class application
	{
	public:
		application( );
		~application( );

		bool initialize( );
		void run( );
		void shutdown( );

		bool is_running( ) const
		{
			return running_;
		}
		void request_exit( )
		{
			running_ = false;
		}

	private:
		bool running_ = false;
		std::unique_ptr< audio_engine > audio_engine_;
		std::unique_ptr< class layout_manager > layout_manager_;

		// GUI state
		GLFWwindow* window_ = nullptr;

		bool init_window( );
		bool init_audio( );
		void main_loop( );
		void render_frame( );
	};

} // namespace pm
