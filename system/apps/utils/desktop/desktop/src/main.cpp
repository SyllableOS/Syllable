#include "main.h"

#include <sys/wait.h>

DeskApp::DeskApp() : Application("application/x-vnd.RGC-desktop_manager")
{

    pcWindow = new BitmapWindow();
    pcWindow->Show();
    pcWindow->MakeFocus();


}

DeskApp::~DeskApp()
{}



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

























































































































