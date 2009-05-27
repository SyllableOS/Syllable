#include "mainwindow.h"
#include "messages.h"
#include "commonfuncs.h"
#include "shutdown.h"
#include "keymap.h"

#include <crypt.h>
#include <gui/requesters.h>

#include "resources/dlogin.h"

using namespace os;


MainWindow::MainWindow( const Rect& cRect, AppSettings* pcAppSettings )
	: os::Window(cRect,"login_window", MSG_MAINWND_TITLE, WND_NO_BORDER | WND_BACKMOST, ALL_DESKTOPS )
{
	/* Most of the work is done in Init() */
	
	pcSettings = pcAppSettings;
}

void MainWindow::Init()
{

	/*Get Resolution and divide width and height by two*/
	IPoint cPoint = GetResolution();
	float vWidth = cPoint.x/2;
	float vHeight = cPoint.y/2;
	
	/*init both the login view and the login imageview*/
	pcLoginImageView = new LoginImageView(GetBounds());
	pcLoginView = new LoginView(Rect(vWidth-200,vHeight-75,vWidth+200,vHeight+75),this, pcSettings);

	/*add both to the window, then add a timer to update the time*/
	AddChild(pcLoginImageView);
	AddChild(pcLoginView);
	AddTimer( this, 12345, 1000000, false );  //update every second
	
	pcLoginView->Focus();
}

void MainWindow::ScreenModeChanged( const os::IPoint& cRes, os::color_space eSpace )
{
	/*resize imageview, resize background, then paint whole view again*/
	pcLoginImageView->ResizeTo(os::Point(cRes));
	pcLoginImageView->ResizeBackground();
	pcLoginImageView->Paint(pcLoginImageView->GetBounds());
	
	/*update the login view*/
	pcLoginView->FrameSized(os::Point(cRes));
	
	/*make sure to move it to the center*/
	float vWidth = cRes.x/2;
	float vHeight = cRes.y/2;
	pcLoginView->MoveTo(vWidth-200,vHeight-75);
	
	/*resize the window to fit everything*/
	ResizeTo(os::Point(cRes));
}

void MainWindow::TimerTick(int nID)
{
	if (nID == 12345)
	{
		pcLoginView->UpdateTime();
	}
}


void MainWindow::HandleMessage( os::Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
		case M_LOGIN:
		{
			/*time to try to log in*/
			String cUser;
			String cPass;
			
			if (pcLoginView->GetUserNameAndPass(&cUser,&cPass) == true) /*we can log in now*/
			{
				Authorize( cUser, cPass, pcLoginView->GetKeymap() );
			}
			
			else /*a user is not selected*/
			{
				Alert* pcError = new Alert(MSG_ALERT_NOTSELECTED_TITLE, MSG_ALERT_NOTSELECTED_TEXT,Alert::ALERT_INFO,0,MSG_ALERT_NOTSELECTED_OK.c_str(),NULL);
				pcError->Go(new Invoker());
			}
			
			break;
		}
		
		case M_SHUTDOWN:
		{
			/*call shutdown window*/
			ShutdownWindow* pcShutdownWindow = new ShutdownWindow(this);
			pcShutdownWindow->CenterInWindow(this);
			pcShutdownWindow->Show();
			pcShutdownWindow->MakeFocus();
			break;
		}
		
		case M_BAD_PASS:
		{
			/*invalid password*/
        	pcLoginView->ClearPassword();
        	pcLoginView->Focus();
        	break;
        }
        
        case M_SEL_CHANGED:
        {
			/*a user has selected a different icon*/
//        	pcLoginView->Focus();
			
			/* Let the login view find the right keymap */
			pcLoginView->HandleMessage( pcMessage );

        	break;
        }
        
		case KeymapSelector::M_SELECT:
		{
//			printf("%s\n", pcLoginView->GetKeymap().c_str());
			os::Application::GetInstance()->SetKeymap(pcLoginView->GetKeymap().c_str());
			break;
		}
	}
}

bool MainWindow::OkToQuit()
{
	return( false );
}

void MainWindow::ClearPassword()
{
	pcLoginView->ClearPassword();
}

void MainWindow::Authorize( const String& zLoginName, const String& zPassword, const String& zKeymap )
{
	/* We pass the keymap here because we only want to save the keymap with the user once they have authenticated themself. */
	os::Message cMsg( M_LOGIN );
	cMsg.AddString( "login", zLoginName );
	cMsg.AddString( "password", zPassword );
	cMsg.AddString( "keymap", zKeymap );
	Application::GetInstance()->PostMessage( &cMsg, Application::GetInstance() );
}






