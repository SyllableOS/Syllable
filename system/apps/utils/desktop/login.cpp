#include <stdio.h>
#include <atheos/kernel.h>

#include <gui/window.h>
#include <gui/view.h>
#include <gui/tableview.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/desktop.h>
#include <util/message.h>

using namespace os;

#define ID_OK 1
#define ID_CANCEL 2

static bool g_bRun      = true;
static bool g_bSelected = false;

static std::string g_cName;
static std::string g_cPassword;

class LoginView : public View
{
public:
    LoginView( const Rect& cFrame );
    ~LoginView();
  
    virtual void AllAttached();
    virtual void FrameSized( const Point& cDelta );
  
private:
    void Layout();

    TextView*	m_pcNameView;
    StringView*	m_pcNameLabel;
    TextView*	m_pcPasswordView;
    StringView*	m_pcPasswordLabel;
    Button*	m_pcOkBut;
//    Button*	m_pcCancelBut;
};

class LoginWindow : public Window
{
public:
    LoginWindow( const Rect& cFrame );

  
    virtual bool	OkToQuit() { g_bRun = false; return( true ); }
    virtual void	HandleMessage( Message* pcMessage );
private:
    LoginView* m_pcView;
};

LoginWindow::LoginWindow( const Rect& cFrame ) : Window( cFrame, "login_window", "Login:", WND_NO_CLOSE_BUT )
{
    m_pcView = new LoginView( GetBounds() );
    AddChild( m_pcView );

}

void LoginWindow::HandleMessage( Message* pcMsg )
{
  switch( pcMsg->GetCode() )
  {
    case ID_OK:
      g_bSelected = true;
    case ID_CANCEL:
      PostMessage( M_QUIT );
      break;
    default:
      Window::HandleMessage( pcMsg );
      break;
  }
}

LoginView::LoginView( const Rect& cFrame ) : View( cFrame, "password_view", CF_FOLLOW_ALL )
{
    m_pcOkBut        = new Button( Rect( 0, 0, 0, 0 ), "ok_but", "Login", new Message( ID_OK ), CF_FOLLOW_NONE );
//    m_pcCancelBut    = new Button( Rect( 0, 0, 0, 0 ), "cancel_but", "Cancel", new Message( ID_CANCEL ), CF_FOLLOW_NONE );
    m_pcNameView     = new TextView( Rect( 0, 0, 0, 0 ), "name_view", "", CF_FOLLOW_NONE );
    m_pcPasswordView = new TextView( Rect( 0, 0, 0, 0 ), "pasw_view", "", CF_FOLLOW_NONE );

    m_pcNameLabel	   = new StringView( Rect(0,0,1,1), "string", "Login name:", ALIGN_RIGHT );
    m_pcPasswordLabel= new StringView( Rect(0,0,1,1), "string", "Password:", ALIGN_RIGHT );

    m_pcPasswordView->SetPasswordMode( true );
  
    AddChild( m_pcNameView, true );
    AddChild( m_pcNameLabel );
    AddChild( m_pcPasswordView, true );
    AddChild( m_pcPasswordLabel );
    AddChild( m_pcOkBut, true );
//    AddChild( m_pcCancelBut, true );

    Layout();
}

LoginView::~LoginView()
{
  g_cName     = m_pcNameView->GetBuffer()[0];
  g_cPassword = m_pcPasswordView->GetBuffer()[0];
}

void LoginView::AllAttached()
{
  m_pcNameView->MakeFocus();
  GetWindow()->SetDefaultButton( m_pcOkBut );
}


void LoginView::FrameSized( const Point& cDelta )
{
    Layout();
}


void LoginView::Layout()
{
    Rect cBounds = GetBounds();

    Rect cButFrame(0,0,0,0);
    Rect cLabelFrame(0,0,0,0);
  
    Point cButSize   = m_pcOkBut->GetPreferredSize( false );
    Point cLabelSize = m_pcNameLabel->GetPreferredSize( false );
    Point cEditSize  = m_pcNameView->GetPreferredSize( false );

    if ( cEditSize.y > cLabelSize.y ) {
	cLabelSize.y = cEditSize.y;
    }
  
    cButFrame.right = cButSize.x - 1;
    cButFrame.bottom = cButSize.y - 1;

    cLabelFrame.right = cLabelSize.x - 1;
    cLabelFrame.bottom = cLabelSize.y - 1;

    Rect cEditFrame( cLabelFrame.right + 10, cLabelFrame.top, cBounds.right - 20, cLabelFrame.bottom );

    m_pcNameLabel->SetFrame( cLabelFrame + Point( 10, 20 ) );
    m_pcNameView->SetFrame( cEditFrame + Point( 10, 20 ) );

    m_pcPasswordLabel->SetFrame( cLabelFrame + Point( 10, 50 + cLabelSize.y  ) );
    m_pcPasswordView->SetFrame( cEditFrame + Point( 10, 50 + cLabelSize.y ) );
  
    m_pcOkBut->SetFrame( cButFrame + Point( cBounds.right - cButSize.x - 15, cBounds.bottom - cButSize.y - 10 ) );
//    m_pcCancelBut->SetFrame( cButFrame + Point( cBounds.right - cButSize.x*2 - 30, cBounds.bottom - cButSize.y - 10 ) );
}


bool get_login( std::string* pcName, std::string* pcPassword )
{
    g_bRun = true;
    g_bSelected = false;

    Rect cFrame( 0, 0, 249, 129 );

    IPoint cScreenRes;

      // Need a new scope to reduce the time the desktop is locked.
    { cScreenRes = Desktop().GetResolution();  }
  
    cFrame += Point( cScreenRes.x / 2 - (cFrame.Width()+1.0f) / 2, cScreenRes.y / 2 - (cFrame.Height()+1.0f) / 2 );
    
    Window* pcWnd = new LoginWindow( cFrame );

    pcWnd->Show();
    pcWnd->MakeFocus();

    while( g_bRun ) {
	snooze( 20000 );
    }
    *pcName     = g_cName;
    *pcPassword = g_cPassword;
    return( g_bSelected );
}




