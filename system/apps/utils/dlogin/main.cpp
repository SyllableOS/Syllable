#include "application.h"

int main( int argc, char *argv[] )
{
	App *pcApp = new App( argc, argv );

	pcApp->Run();
	return ( 0 );
}
