#include "login.h"


/*
** name:       WriteLoginConfigFile
** purpose:    Writes a config file to /boot/atheos/sys/config/login.cfg; if one does
**			   not exsist.
** parameters: 
** returns:	   
*/
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



/*
** name:       CheckLoginConfig
** purpose:    Checks to see if /boot/atheos/sys/config/login.cfg exsists; if it doesn't
**			   exsist.
** parameters: 
** returns:	   
*/
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


/*
** name:       ReadLoginOption
** purpose:    Reads the login config file
** parameters: 
** returns:	   const char* containning what will be placed in the login name textview
*/
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

    if (strcmp(login_info,"true") == 0)
    {
        return_name = login_name;
    }

    else
    {return_name = "\n";}

    return (return_name);
}




/*
** name:       LoginView Constructor
** purpose:    Constructs the LoginView
** parameters: Rect that contains the size of the view
** returns:	   
*/
LoginView::LoginView( const Rect& cFrame ) : View( cFrame, "password_view", CF_FOLLOW_NONE )
{
    Color32_s bg(239,236,231,255);

    m_pcOkBut        = new ColorButton( Rect( 0, 0, 0, 0 ),"ok", "Login", new Message( ID_OK ),bg);
    m_pcNameView     = new TextView( Rect( 0, 0, 0, 0 ), "name_view", "", CF_FOLLOW_NONE );
    m_pcPasswordView = new TextView( Rect( 0, 0, 0, 0 ), "pasw_view", "", CF_FOLLOW_NONE );

    m_pcPasswordView->SetPasswordMode( true );

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


/*
** name:       LoginView::Paint
** purpose:    Paints LoginView to the screen
** parameters: Rect that contains the size of the view
** returns:	   
*/
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


/*
** name:       LoginView Destructor
** purpose:    Needed to pass the name and the password to main
** parameters: 
** returns:	   
*/
LoginView::~LoginView()
{
    g_cName     = m_pcNameView->GetBuffer()[0];
    g_cPassword = m_pcPasswordView->GetBuffer()[0];
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
    m_pcNameView->SetFrame(Rect(0,0,170,25) + Point(290,35));
    m_pcPasswordView->SetFrame(Rect(0,0,170,25) + Point(290,85));
    m_pcOkBut->SetFrame(Rect(0,0,60,25) + Point(400,120));


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
    pcLoginImage = LoadBitmapFromResource("syllable.jpg");
    pcAtheImage = LoadBitmapFromResource("logo_atheos.jpg");
}


/*
** name:       LoginWindow Constructor
** purpose:    Constructs the LoginWindow
** parameters: Rect(determinies the size of the window)
** returns:	   
*/
LoginWindow::LoginWindow( const Rect& cFrame ) : Window( cFrame, "login_window", "Login:", WND_NO_BORDER )
{
    Rect cRect(0,0,470,195);
    m_pcView = new LoginView(cRect);
    m_pcView->FillRect(cRect, Color32_s(239,236,231));
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
        g_bSelected = true;
    case ID_CANCEL:
        PostMessage( M_QUIT );
        break;
    default:
        Window::HandleMessage( pcMsg );
        break;
    }
}


/*
** name:       get_login
** purpose:    
** parameters: string(for the name of the user), string(for the password of the user)
** returns:	   bool
*/
bool get_login( std::string* pcName, std::string* pcPassword )
{
    g_bRun = true;
    g_bSelected = false;
    Rect cFrame( 0, 0,470,195 );  //470 195

    IPoint cScreenRes;

    // Need a new scope to reduce the time the desktop is locked.
    { cScreenRes = Desktop().GetResolution();  }

    cFrame += Point( cScreenRes.x / 2 - (cFrame.Width()+1.0f) / 2, cScreenRes.y / 2 - (cFrame.Height()+1.0f) / 2 );

    Window* pcWnd = new LoginWindow( cFrame ); // cFrame

    pcWnd->Show();
    pcWnd->MakeFocus();

    while( g_bRun )
    {
        snooze( 20000 );
    }
    *pcName     = g_cName;
    *pcPassword = g_cPassword;
    return( g_bSelected );
}



























