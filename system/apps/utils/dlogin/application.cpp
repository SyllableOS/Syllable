#include "application.h"
#include "commonfuncs.h"
#include "messages.h"
#include <gui/requesters.h>

#include "resources/dlogin.h"

using namespace os;


App::App( int nArgc, char* apzArgv[] ) :os::Application( "application/x-vnd.syllable-login_application" )
{
	SetCatalog("dlogin.catalog");

	mkdir("/system/icons/users",S_IRWXU | S_IRWXG | S_IRWXO);
//	SetPublic(true);
	
	m_pcSettings = new AppSettings();
	
	if( m_pcSettings->IsAutoLoginEnabled() )
	{
		AutoLogin( m_pcSettings->GetAutoLoginUser() );
		/* If the login is successful, we will waitpid() on the user's desktop. */
		/* Once the desktop has quit, we will resume from here and create the login window as usual. */
		
		/* Should this be done in App::Started() instead? So that the Application is fully initialised */
	}

	IPoint cPoint = GetResolution();
	m_pcMainWindow = new MainWindow( Rect(0,0,cPoint.x,cPoint.y), m_pcSettings );
	m_pcMainWindow->Init();
	m_pcMainWindow->Show();
	m_pcMainWindow->MakeFocus();
}

AppSettings* App::GetSettings()
{
	return( m_pcSettings );
}

void App::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_LOGIN:
		{
			os::String cLogin;
			os::String cPassword;
			os::String cKeymap = "";
			pcMessage->FindString( "login", &cLogin );
			pcMessage->FindString( "password", &cPassword );
			pcMessage->FindString( "keymap", &cKeymap );
			Authorize( cLogin, cPassword, cKeymap );
			break;
		}
		default:
			Application::HandleMessage(pcMessage);
	}
}

void App::Authorize( const String& zUserName, const String& zPassword, const String& zKeymap )
{
	String zLoginName = zUserName;
    for (;;)	/* AWM: What is this loop for? */
    {

        if (zLoginName != "")
        {
            struct passwd* psPass;

            psPass = getpwnam( zLoginName.c_str() );


            if ( psPass != NULL )
            {
				const char* pzPassWd = crypt(  zPassword.c_str(), "$1$" );

                if ( zLoginName != "" && pzPassWd == NULL )
                {
                    perror( "crypt()" );
                    zLoginName = "";
                    continue;
                }

                if (strcmp( pzPassWd, psPass->pw_passwd ) == 0 )
                {
                	/* Password is correct */
                	
                	/* Save the user & keymap setting */
                	m_pcSettings->SetKeymapForUser( zLoginName, zKeymap );
                	m_pcSettings->SetActiveUser( zLoginName );
                	m_pcSettings->Save();
                	
					/* Let's become this user */
					Unlock();
					m_pcMainWindow->Hide();
					m_pcMainWindow->Terminate();
					Lock();
					BecomeUser(psPass);
					IPoint cPoint = GetResolution();
					m_pcMainWindow = new MainWindow( Rect(0,0,cPoint.x,cPoint.y), m_pcSettings );
					m_pcMainWindow->Init();
					m_pcMainWindow->Show();
					m_pcMainWindow->MakeFocus();
					break;
                }
                else
                {
					/*passwords don't match... flag error*/
                    Alert* pcAlert = new Alert( MSG_ALERT_WRONGPASS_TITLE, MSG_ALERT_WRONGPASS_TEXT+"\n",Alert::ALERT_WARNING,0, MSG_ALERT_WRONGPASS_OK.c_str(), NULL );
                    pcAlert->Go(new Invoker(new Message (M_BAD_PASS), m_pcMainWindow));
                }
                break;
            }
            else
            {
				/*no such user available*/
                Alert* pcAlert = new Alert( MSG_ALERT_NOUSER_TITLE, MSG_ALERT_NOUSER_TEXT+"\n",Alert::ALERT_WARNING, 0, MSG_ALERT_NOUSER_OK.c_str(), NULL );
                pcAlert->Go(new Invoker());
            }
            break;
        }
        zLoginName = "";
    }
}


void App::AutoLogin( const String& zUserName )
{
	struct passwd* psPass;
	psPass = getpwnam( zUserName.c_str() );

	if( psPass != NULL ) {
		BecomeUser(psPass);
	}
}



