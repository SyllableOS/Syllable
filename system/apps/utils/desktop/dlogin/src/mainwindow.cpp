#include "mainwindow.h"
#include "commonfuncs.h"
#include "messages.h"
using namespace os;



/*
** name:       LoginView Constructor
** purpose:    Constructs the LoginView
** parameters: Rect that contains the size of the view
** returns:	   
*/
LoginView::LoginView( const Rect& cFrame ) : View( cFrame, "password_view", CF_FOLLOW_NONE )
{
    LoadImages();
   	Color32_s bg(239,236,231,255);

    m_pcOkBut        = new ColorButton( Rect( 0, 0, 0, 0 ),"ok", "_Login", new Message( ID_OK ),bg);
	m_pcNameView     = new TextView( Rect( 0, 0, 0, 0 ), "name_view", "", CF_FOLLOW_NONE );
    m_pcPasswordView = new TextView( Rect( 0, 0, 0, 0 ), "pasw_view", "", CF_FOLLOW_NONE );

    m_pcPasswordView->SetPasswordMode( true );

    AddChild( m_pcNameView);
    AddChild( m_pcPasswordView);
    AddChild( m_pcOkBut);

    Layout();


    m_pcNameView->SetTabOrder(0);
    m_pcPasswordView->SetTabOrder(1);
    m_pcOkBut->SetTabOrder(2);

	Paint(GetBounds());
	Flush();
	Sync();
	Invalidate();
}


/*
** name:       LoginView::Paint
** purpose:    Paints LoginView to the screen
** parameters: Rect that contains the size of the view
** returns:	   
*/
void LoginView::Paint(const Rect & cUpdate)
{
    os::String zInfo = GetSyllableVersion();

    SetFgColor(102,136,217,0);
	SetBgColor(255,255,255,0);
	SetEraseColor(102,136,217,0);

	SetDrawingMode(DM_OVER);

    pcLoginImage->Draw(Point(0,0),this);

    MovePenTo(470-GetStringWidth(zInfo.c_str()),40);
    DrawString(zInfo.c_str());

}



/*
** name:       LoginView Destructor
** purpose:    Needed to pass the name and the password to main
** parameters: 
** returns:	   
*/
LoginView::~LoginView()
{
    delete pcLoginImage;
	delete pcShutdownImage;
    delete m_pcPasswordView;
}


/*
** name:       LoginView::AllAttached
** purpose:    Executed after the LoginView is attached to the window
** parameters: 
** returns:	   
*/
void LoginView::AllAttached()
{
    m_pcNameView->MakeFocus();
    GetWindow()->SetDefaultButton( m_pcOkBut );
}



/*
** name:       LoginView::FramSized
** purpose:    
** parameters: 
** returns:	   
*/
void LoginView::FrameSized( const Point& cDelta )
{
    Layout();
}



/*
** name:       LoginView::Layout
** purpose:    Lays out everthing in the LoginView
** parameters: 
** returns:	   
*/
void LoginView::Layout()
{
    m_pcNameView->SetFrame(Rect(0,0,170,25) + Point(270,175));  //290
    m_pcPasswordView->SetFrame(Rect(0,0,170,25) + Point(270,204));
    m_pcOkBut->SetFrame(Rect(0,0,65,25) + Point(375,240));
    m_pcNameView->Set(ReadLoginOption(),true);
}


/*
** name:       LoginView::LoadImages
** purpose:    Used to initialize the images used for the LoginView
** parameters: 
** returns:	   
*/
void LoginView::LoadImages()
{
    pcLoginImage = LoadImageFromResource("syllable.gif");
}




/*
** name:       LoginWindow Constructor
** purpose:    Constructs the LoginWindow
** parameters: Rect(determinies the size of the window)
** returns:	   
*/
LoginWindow::LoginWindow( const Rect& cFrame ) : Window( cFrame, "login_window", "Login:",WND_NO_BORDER | WND_BACKMOST )
{
    Rect cRect(0,0,500,300);
    m_pcView = new LoginView(cRect);
    AddChild( m_pcView );

    CheckLoginConfig();


    if (!strcmp(ReadLoginOption(),"\n") == 0)
        SetFocusChild(m_pcView->m_pcPasswordView);


    else
        SetFocusChild(m_pcView->m_pcNameView);


}


/*
** name:       LoginWindow::HandleMessage
** purpose:    Handles messages between the window and th gui components.
** parameters: Message(the message that is passed from gui component to the window)
** returns:	   
*/
void LoginWindow::HandleMessage( Message* pcMsg )
{
    switch( pcMsg->GetCode() )
    {
    case ID_OK:
        pzLogin = 	m_pcView->m_pcNameView->GetBuffer()[0];
        cPassword = m_pcView->m_pcPasswordView->GetBuffer()[0];
        Authorize(pzLogin.c_str());
        break;
    case ID_CANCEL:
        PostMessage( M_QUIT );
        break;

    case ID_BAD_PASS:
        m_pcView->m_pcPasswordView->Clear();
        SetFocusChild(m_pcView->m_pcPasswordView);
        break;

	case ID_SHUTDOWN:
		LaunchShutdown();
		break;

    default:
        Window::HandleMessage( pcMsg );
        break;
    }
}



LoginWindow::~LoginWindow()
{
}

void LoginWindow::LaunchShutdown()
{
	/*m_pcShutdownWindow = new ShutdownWindow();
	m_pcShutdownWindow->CenterInWindow(this);
	m_pcShutdownWindow->Show();
	m_pcShutdownWindow->MakeFocus();*/
}

void LoginWindow::LaunchOptions()
{
}

void LoginWindow::Authorize( const char* pzLoginName )
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
                const char* pzPassWd = crypt(  cPassword.c_str(), "$1$" );

                if ( pzLoginName == NULL && pzPassWd == NULL )
                {
                    perror( "crypt()" );
                    pzLoginName = NULL;
                    continue;
                }

                if (strcmp( pzPassWd, psPass->pw_passwd ) == 0 )
                {
                    if (BecomeUser(psPass,this));
					break;
                }
                else
                {
                    Alert* pcAlert = new Alert( "Login failed",  "Incorrect password!!!\n",Alert::ALERT_WARNING,0, "Sorry", NULL );
                    pcAlert->Go(new Invoker(new Message (ID_BAD_PASS), this));
                }
                break;
            }
            else
            {

                Alert* pcAlert = new Alert( "Login failed",  "Please be advised!!!\n\nNo such user is\nregistered to use\nthis computer!!!\n",Alert::ALERT_WARNING, 0, "OK", NULL );
                pcAlert->Go(new Invoker());
            }
            break;
        }
        pzLoginName = NULL;
    }
}
