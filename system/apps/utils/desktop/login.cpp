#include "login.h"

void WriteLoginConfigFile()
{
    FILE* fin;
    fin = fopen("/boot/atheos/sys/config/login.cfg","w");
    
    fprintf(fin,"<Login Name Option>\n");
    fprintf(fin,"false\n\n");
    fprintf(fin, "<Login Name>\n");
    fprintf(fin,"\n");
    fclose(fin);
}




void CheckLoginConfig()
{
	ifstream filestr;
    filestr.open("/boot/atheos/sys/config/login.cfg");

    if(filestr == NULL)
    {
        filestr.close();
        WriteLoginConfigFile();
    }

    else
    {
        filestr.close();
    }
}

const char* ReadLoginOption()
{
    char junk[1024];
    char login_info[1024];
    char login_name[1024];
    const char* return_name;
    
    ifstream filRead;
    filRead.open("/boot/atheos/sys/config/login.cfg");
    
    filRead.getline(junk,1024);
    filRead.getline((char*)login_info,1024);
    filRead.getline(junk,1024);
    filRead.getline(junk,1024);
    filRead.getline((char*)login_name,1024);
    
    filRead.close();
    
   if (strcmp(login_info,"true") == 0){
   	return_name = login_name;
   	}
   	
   	else{return_name = "\n";}
   			
   return (return_name);
}


class LoginView : public View
{
public:
    LoginView( const Rect& cFrame );
    ~LoginView();

    virtual void AllAttached();
    virtual void FrameSized( const Point& cDelta );
    TextView*	m_pcNameView;
    StringView*	m_pcNameLabel;
    TextView*	m_pcPasswordView;
    StringView*	m_pcPasswordLabel;
    ColorButton*	m_pcOkBut;
    

private:
    void Layout();
    virtual void Paint(const Rect& cUpdate);
    bool          Read();
    void          LoadImages();
    Bitmap*       pcLoginImage;
    Bitmap*		  pcAtheImage;
   
};


LoginView::LoginView( const Rect& cFrame ) : View( cFrame, "password_view", CF_FOLLOW_NONE )
{
    Color32_s bg(239,236,231,255);
    //Color32_s fg(0,0,0,255);     
    m_pcOkBut        = new ColorButton( Rect( 0, 0, 0, 0 ),"ok", "Login", new Message( ID_OK ),bg);
    m_pcNameView     = new TextView( Rect( 0, 0, 0, 0 ), "name_view", "", CF_FOLLOW_NONE );
    m_pcPasswordView = new TextView( Rect( 0, 0, 0, 0 ), "pasw_view", "", CF_FOLLOW_NONE );

	

    m_pcPasswordView->SetPasswordMode( true );

    //m_pcOkBut->SetFgColor(0,0,0);
    //m_pcOkBut->SetBgColor(239,236,231);

    AddChild( m_pcNameView, true );
    AddChild( m_pcPasswordView, true );
    AddChild( m_pcOkBut, true );


    Layout();
    LoadImages();
    Paint(GetBounds());
    Invalidate();
    
    m_pcNameView->SetTabOrder(0);
    m_pcPasswordView->SetTabOrder(1);
    m_pcOkBut->SetTabOrder(2);
    
    
    
    
}



void LoginView::Paint(const Rect & cUpdate)
{
    FillRect(cUpdate, Color32_s(239,236,231,0));
    SetFgColor(0,0,0);

    String sLogin = "Login Name:";
    String sPass = "Password:";
    String sWarning = "Hint: Hitting Ctrl-Alt-Delete will reboot the machine";
    font_height sHight;

    float vStrWidth = GetStringWidth(sLogin);
    float f_warning, f_login;
    GetFontHeight(&sHight);

    f_warning = GetBounds().Width() -250.0f - vStrWidth;
    f_login = GetBounds().Width() - 190 - vStrWidth;


    DrawString(sLogin, Point(f_login, (GetBounds().Height()-100.0f)/2 - (sHight.ascender + sHight.descender)/2 + sHight.ascender) );
    DrawString(sPass, Point(f_login, (GetBounds().Height()-0.0f)/2 - (sHight.ascender + sHight.descender)/2 + sHight.ascender) );
    DrawString(sWarning, Point(f_warning, (GetBounds().Height()+180.0f) /2 - (sHight.ascender - sHight.descender)/2 + sHight.ascender));

    DrawBitmap(pcLoginImage,pcLoginImage->GetBounds(),Rect(0,0,380,45));
    DrawBitmap(pcAtheImage,pcAtheImage->GetBounds(),Rect(45,0,164,140));
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
    m_pcNameView->SetFrame(Rect(0,0,170,25) + Point(290,35));
    m_pcPasswordView->SetFrame(Rect(0,0,170,25) + Point(290,85));
    m_pcOkBut->SetFrame(Rect(0,0,60,25) + Point(400,120));
    
    
    m_pcNameView->Set(ReadLoginOption(),true);
}

void LoginView::LoadImages()
{
    FILE* f_uname = popen("uname -v 2>&1", "r");
    char pzUname[1024];
    const char* pzUnamePrint = fgets(pzUname, sizeof(pzUname),f_uname);

    if( strstr(pzUnamePrint, "0.3.7")){
        pcLoginImage = LoadBitmapFromResource("atheos_0.3.7.jpg");
    }

    else if (strstr(pzUnamePrint, "0.3.6"))
    {
        pcLoginImage = LoadBitmapFromResource("atheos_0.3.6.jpg");
    }

    else if(strstr(pzUnamePrint, "0.3.5"))
    {
        pcLoginImage = LoadBitmapFromResource("atheos_0.3.5.jpg");
    }

    else
    {
        pcLoginImage = LoadBitmapFromResource("atheos_0.undetermined.jpg");
    }

    pcAtheImage = LoadBitmapFromResource("logo_atheos.jpg");
}

class LoginWindow : public Window
{
public:
    LoginWindow( const Rect& cFrame );
    virtual bool	OkToQuit() { g_bRun = false; return( true ); }
    virtual void	HandleMessage( Message* pcMessage );
private:
    LoginView* m_pcView;
};

LoginWindow::LoginWindow( const Rect& cFrame ) : Window( cFrame, "login_window", "Login:", WND_NO_BORDER )
{
    m_pcView = new LoginView( GetBounds() );
    m_pcView->FillRect(cFrame, Color32_s(239,236,231));
    AddChild( m_pcView );
    
    CheckLoginConfig();
    
   
    if (!strcmp(ReadLoginOption(),"\n") == 0)
    	SetFocusChild(m_pcView->m_pcPasswordView);
    	
    	
    else
    	SetFocusChild(m_pcView->m_pcNameView);
    	
    	
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



bool get_login( std::string* pcName, std::string* pcPassword )
{
    g_bRun = true;
    g_bSelected = false;
    Rect cFrame( 0, 0,470,195 );  //470 195

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























