#include "application.h"

int main( int argc, char *argv[] )
{
	if( argc < 2 )
	{
		App* pcApp = new App( "empty" );
		pcApp->Run();
		return( 0 );
	}
	else
	{
		App* pcApp = new App( argv[1] );
		pcApp->Run();
		return( 0 );
	}
}

