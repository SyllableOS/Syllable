#include "application.h"

int main( int argc, char *argv[] )
{
	App* pcApp = NULL;
	if( argc > 1 )
		pcApp = new App( argv[1], true );
	else
		pcApp = new App( "", false );
	pcApp->Run();
	return( 0 );
}

