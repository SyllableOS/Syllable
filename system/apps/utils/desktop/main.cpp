#include "main.h"
#include "debug.h"

DeskApp::DeskApp() : Application("application/x-vnd.RGC-desktop_manager")
{
	
	pcLogin = new LoginWindow(CRect(470,195));
	pcLogin->Show();
    pcLogin->MakeFocus();
    
    Debug("DeskApp::DeskApp() : just showed the Login Window");
}

DeskApp::~DeskApp()
{
	
}
	 


/*
** name:       main
** purpose:    main function
** parameters: int and char**
** returns:    
*/
int main( int argc, char** argv )
{
    Debug("main( int argc, char** argv) : just opened up the main application");
    DeskApp* pcApp = new DeskApp();
   
    pcApp->Run();

	
    return( 0 );
}









































































































