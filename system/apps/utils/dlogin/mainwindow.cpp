#include "mainwindow.h"
#include "messages.h"
#include "commonfuncs.h"
#include "shutdown.h"

#include <crypt.h>
#include <gui/requesters.h>

using namespace os;


MainWindow::MainWindow(const Rect& cRect) : os::Window(cRect,"login_window", "Login:",WND_NO_BORDER | WND_BACKMOST )
{
	/*do nothing in here, let init do all the work*/
}

void MainWindow::Init()
{

	/*Get Resolution and divide width and height by two*/
	IPoint cPoint = GetResolution();
	float vWidth = cPoint.x/2;
	float vHeight = cPoint.y/2;
	
	/*init both the login view and the login imageview*/
	pcLoginImageView = new LoginImageView(GetBounds());
	pcLoginView = new LoginView(Rect(vWidth-200,vHeight-75,vWidth+200,vHeight+75),this);

	/*add both to the window, then add a timer to update the time*/
	AddChild(pcLoginImageView);
	AddChild(pcLoginView);
	AddTimer( this, 12345, 1000000, false );  //update every second
	
	/*focus the login view*/
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
				Authorize(cUser.c_str(),cPass.c_str());
			}
			
			else /*a user is not selected*/
			{
				Alert* pcError = new Alert("Select an icon please!!!","You must select an user icon before loginning in!",Alert::ALERT_INFO,0,"Okay",NULL);
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
        	pcLoginView->Focus();
        	break;
        }
	}
}

bool MainWindow::OkToQuit()
{
	Application::GetInstance()->PostMessage(M_QUIT );
	return( true );
}

void MainWindow::ClearPassword()
{
	pcLoginView->ClearPassword();
}

void MainWindow::Authorize(const char* pzLoginName, const char* pzPassword )
{
    for (;;)
    {

        if (pzLoginName != NULL)
        {
            struct passwd* psPass;

            if ( pzLoginName != NULL )
            {
                psPass = getpwnam( pzLoginName );
            }
            else
            {
                psPass = getpwnam( pzLoginName );
            }

            if ( psPass != NULL )
            {
                const char* pzPassWd = crypt(  pzPassword, "$1$" );

                if ( pzLoginName == NULL && pzPassWd == NULL )
                {
                    perror( "crypt()" );
                    pzLoginName = NULL;
                    continue;
                }

                if (strcmp( pzPassWd, psPass->pw_passwd ) == 0 )
                {
					/*passwords match, lets become this user*/
                    if (BecomeUser(psPass,this));
					break;
                }
                else
                {
					/*passwords don't match... flag error*/
                    Alert* pcAlert = new Alert( "Login failed",  "Incorrect password!!!\n",Alert::ALERT_WARNING,0, "Sorry", NULL );
                    pcAlert->Go(new Invoker(new Message (M_BAD_PASS), this));
                }
                break;
            }
            else
            {
				/*no such user available*/
                Alert* pcAlert = new Alert( "Login failed",  "Please be advised!!!\n\nNo such user is\nregistered to use\nthis computer!!!\n",Alert::ALERT_WARNING, 0, "OK", NULL );
                pcAlert->Go(new Invoker());
            }
            break;
        }
        pzLoginName = NULL;
    }
}

