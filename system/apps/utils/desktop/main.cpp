#include "main.h"


DeskApp::DeskApp() : Application("application/x-vnd.RGC-desktop_manager")
{
	
	pcLogin = new LoginWindow(CRect(470,195));
	pcLogin->Show();
    pcLogin->MakeFocus();
}
	 


/*
** name:       main
** purpose:    main function
** parameters: int and char**
** returns:    
*/
int main( int argc, char** argv )
{

    DeskApp* pcApp = new DeskApp();
   
    pcApp->Run();

    return( 0 );
}








































































































