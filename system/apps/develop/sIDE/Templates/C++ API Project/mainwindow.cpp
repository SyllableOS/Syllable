#include "mainwindow.h"
#include "messages.h"

MainWindow::MainWindow() : os::Window( os::Rect( 0, 0, 300, 300 ), "main_wnd", "MainWindow" )
{
	SetTitle( "Your Title here" );
}

void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_APP_QUIT:
		{
			OkToQuit();
		}
		break;
	}
}
bool MainWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}
