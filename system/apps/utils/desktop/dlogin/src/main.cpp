#include "main.h"
#include "include.h"


/*Thank you Will for the help :) */
int Become_User( struct passwd *psPwd, LoginWindow* pcWindow ) {
  int nStatus;
  pid_t nError = waitpid(-1, &nStatus, WNOHANG);
  

  switch( (nError = fork()) ) {
    case -1:
      break;

    case 0: /* child process */
    	
      setuid( psPwd->pw_uid );
      setgid( psPwd->pw_gid );
      chdir( psPwd->pw_dir );
      setenv( "HOME", psPwd->pw_dir,true );
      setenv( "USER", psPwd->pw_name,true );
      setenv( "SHELL", psPwd->pw_shell,true );
      setenv( "PATH", "/bin:/atheos/autolnk/bin",true);
      execl( "/bin/desktop", "desktop", NULL );
      
      
      break;
     
    	
    default: /* parent process */
      // hide login box
       pcWindow->Hide();
       pcWindow->m_pcView->m_pcPasswordView->Clear();
       int nDesktopPid, nExitStatus;
     
      nDesktopPid = nError;
     
      nError = waitpid( nDesktopPid, &nExitStatus, 0 );
     
      if( nError < 0 || nError != nDesktopPid ) // Something went wrong ;-)
         break;
     
      // Show the login window again
      sleep(1);
      pcWindow->Show();
      pcWindow->MakeFocus();
      return 0;
      //return nError;
      
  	}
 
 return -errno;
}


/*
** name:       UpdateLoginConfig
** purpose:    Updates /system/config/login.cfg with the newest person who loggd in.
** parameters: A string that represents the newest person
** returns:
*/
void UpdateLoginConfig(string zName)
{
    char junk[1024];
    char login_info[1024];
    char login_name[1024];

    ifstream filRead;
    filRead.open("/boot/atheos/sys/config/login.cfg");

    filRead.getline(junk,1024);
    filRead.getline((char*)login_info,1024);
    filRead.getline(junk,1024);
    filRead.getline(junk,1024);
    filRead.getline((char*)login_name,1024);
    filRead.close();


    FILE* fin;
    fin = fopen("/boot/atheos/sys/config/login.cfg","w");

    fprintf(fin,"<Login Name Option>\n");
    fprintf(fin, login_info);
    fprintf(fin, "\n\n<Login Name>\n");
    fprintf(fin,zName.c_str());
    fprintf(fin,"\n");
    fclose(fin);
}

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

    if (!strcmp(login_info,"true"))
    	return(login_name);
    else
    	return ("\n");
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

    /*if(filestr == NULL)
    {
        filestr.close();
        WriteLoginConfigFile();
    }

    else
    {*/
        filestr.close();
        ReadLoginOption();
    //}
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
    SetDrawingMode(DM_OVER);
    DrawBitmap(pcLoginImage,pcLoginImage->GetBounds(),Rect(0,0,pcLoginImage->GetBounds().Width(),pcLoginImage->GetBounds().Height()));
    
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
    m_pcOkBut->SetFrame(Rect(0,0,60,25) + Point(380,240));


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
    pcLoginImage = LoadBitmapFromResource("syllable.gif");

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
        pzLogin = (string)m_pcView->m_pcNameView->GetBuffer()[0];
        cPassword = (string)m_pcView->m_pcPasswordView->GetBuffer()[0];
        Authorize(pzLogin.c_str());
        break;
    case ID_CANCEL:
        PostMessage( M_QUIT );
        break;
    default:
        Window::HandleMessage( pcMsg );
        break;
    }
}



LoginWindow::~LoginWindow()
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
                	if (Become_User(psPass,this))
                		UpdateLoginConfig(pzLoginName);
                    break;
                }
                else
                {
                    Alert* pcAlert = new Alert( "Login failed:",  "Incorrect password!!!\n",Alert::ALERT_WARNING,0, "Sorry", NULL );
                    pcAlert->Go(new Invoker());
                }
                break;
            }
            else
            {

                Alert* pcAlert = new Alert( "Login failed:",  "Please be advised!!!\n\nNo such user is\nregistered to use\nthis computer!!!\n",Alert::ALERT_WARNING, 0, "OK", NULL );
                pcAlert->Go(new Invoker());
            }
        	break;
        }
        pzLoginName = NULL;
}
}


LoginApp::LoginApp() : Application("application./x-vnd.RGC-desktop-login")
{
	pcWindow = new LoginWindow(CRect(500,300));
	pcWindow->Show();
    pcWindow->MakeFocus();
}


int main()
{
	LoginApp* thisApp;
	thisApp = new LoginApp();
	
	thisApp->Run();
	
}






























































































































