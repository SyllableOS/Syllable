#include "mainwindow.h"
#include "messages.h"

MainWindow::MainWindow() : os::Window( os::Rect( 0, 0, 300, 300 ), "main_wnd", "Your title here" )
{
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
}

void MainWindow::HandleMessage( os::Message* pcMessage )
{
	switch( pcMessage->GetCode() )
	{
		case M_APP_QUIT:
		{
			PostMessage( os::M_QUIT );
		}
		break;
	}
}
bool MainWindow::OkToQuit()  // Obsolete?
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}

