#include "application.h"
#include "commonfuncs.h"
#include "messages.h"
#include <gui/requesters.h>

#include "resources/dlogin.h"

App::App():os::Application( "application/x-vnd.syllable-login_application" )
{
	SetCatalog("dlogin.catalog");

	mkdir("/system/icons/users",S_IRWXU | S_IRWXG | S_IRWXO);
	SetPublic(true);

	IPoint cPoint = GetResolution();
	m_pcMainWindow = new MainWindow(Rect(0,0,cPoint.x,cPoint.y));
	m_pcMainWindow->Init();
	m_pcMainWindow->Show();
	m_pcMainWindow->MakeFocus();
}

void App::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_LOGIN:
		{
			os::String cLogin;
			os::String cPassword;
			pcMessage->FindString( "login", &cLogin );
			pcMessage->FindString( "password", &cPassword );
			Authorize( cLogin.c_str(), cPassword.c_str() );
			break;
		}
		default:
			Application::HandleMessage(pcMessage);
	}
}

void App::Authorize(const char* pzLoginName, const char* pzPassword )
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
					Unlock();
					m_pcMainWindow->Hide();
					m_pcMainWindow->Terminate();
					Lock();
					BecomeUser(psPass);
					IPoint cPoint = GetResolution();
					m_pcMainWindow = new MainWindow(Rect(0,0,cPoint.x,cPoint.y));
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
        pzLoginName = NULL;
    }
}














