#include "src/app/application.h"

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow )
{
	( void )hInstance;
	( void )hPrevInstance;
	( void )lpCmdLine;
	( void )nCmdShow;

	pm::application app;

	if ( !app.initialize( ) ) {
		MessageBoxA( nullptr, "failed to initialize application", "ERROR", MB_OK | MB_ICONERROR );
		return 1;
	}

	app.run( );

	return 0;
}

#else
// somewhat of a redundant fallback
int main( int argc, char** argv )
{
	( void )argc;
	( void )argv;

	pm::application app;

	if ( !app.initialize( ) ) {
		return 1;
	}

	app.run( );

	return 0;
}
#endif
