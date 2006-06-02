#include "application.h"

/*************************************************************
* Description: Main()
* Author: Rick Caudill 
* Date: Sat Oct 23 2004 11:03:07 
*************************************************************/
int main( int argc, char *argv[] )
{
	App* pcApp = new App(argc,argv);
	pcApp->Run();
	return( 0 );
}
