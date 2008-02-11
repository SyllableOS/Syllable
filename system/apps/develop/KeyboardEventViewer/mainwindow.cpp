#include "mainwindow.h"
#include "messages.h"
#include "view.h"

MainWindow::MainWindow() : os::Window( os::Rect( 0, 0, 400, 250 ), "main_wnd", "Keyboard Event Viewer" )
{
	/* Set Icon */
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
	
	
	pcLayout = new os::LayoutView(GetBounds(),"layout");
	os::VLayoutNode* pcRoot = new os::VLayoutNode("root");
	pcRoot->AddChild(pcView = new EventView());
	pcRoot->AddChild(pcRefresh = new os::Button(os::Rect(),"but","Refresh",new os::Message(M_REFRESH)),0.0f);
	pcLayout->SetRoot(pcRoot);
	AddChild(pcLayout);

	pcView->Refresh();
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
		
		case M_REFRESH:
		{
			pcView->Refresh();
		}
		break;
	}
}
bool MainWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return( true );
}











