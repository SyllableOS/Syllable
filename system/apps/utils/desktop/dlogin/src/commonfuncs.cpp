#include "commonfuncs.h"
#include "mainwindow.h"



using std::ifstream;

int BecomeUser( struct passwd *psPwd, LoginWindow* pcWindow )
{
	int nStatus;
    pid_t nError = waitpid(-1, &nStatus, WNOHANG);

    switch( (nError = fork()) )
    {
    case -1:
        break;

    case 0: /* child process */
        setuid( psPwd->pw_uid );
        setgid( psPwd->pw_gid );
        chdir( psPwd->pw_dir );
        setenv( "HOME", psPwd->pw_dir,true );
        setenv( "USER", psPwd->pw_name,true );
        setenv( "SHELL", psPwd->pw_shell,true );
        UpdateLoginConfig(psPwd->pw_name);
		 Application::GetInstance()->PopCursor();
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

        sleep(1);
        pcWindow->Show();
        pcWindow->MakeFocus();
        return 0;
    }
    return -errno;
}


/*
** name:       UpdateLoginConfig
** purpose:    Updates /system/config/login.cfg with the newest person who logged in.
** parameters: A string that represents the newest person
** returns:
*/
void UpdateLoginConfig(os::String zName)
{
	FSNode* pcNode = new FSNode("/bin/dlogin");
	pcNode->RemoveAttr("user");
	pcNode->WriteAttr("user",0,ATTR_TYPE_STRING,zName.c_str(),0,zName.size());
	delete pcNode;	
}

/*
** name:       ReadLoginOption
** purpose:    Reads the login config file
** parameters: 
** returns:	   const char* containning what will be placed in the login name textview
*/
const char* ReadLoginOption()
{
	char zUser[1024]= {NULL,};
	FSNode* pcNode = new FSNode("/bin/dlogin");
	pcNode->ReadAttr("user",ATTR_TYPE_STRING,zUser,0,1024);
	delete pcNode;
	return zUser;
}



os::String GetSyllableVersion()
{
	String cReturn;
	system_info sSysInfo;
	get_system_info(&sSysInfo);
	cReturn.Format("%s%d.%d.%d", "Syllable v",( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) % 0xffff ), ( int )( sSysInfo.nKernelVersion & 0xffff ));	
	return cReturn;
}

/************************************************
* Description: Loads a bitmapimage from resource
* Author: Rick Caudill
* Date: Thu Mar 18 21:32:37 2004
*************************************************/
os::BitmapImage* LoadImageFromResource( const os::String &cName )
{
	os::BitmapImage * pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	os::ResStream * pcStream = cRes.GetResourceStream( cName.c_str() );
	if( pcStream == NULL )
	{
		throw( os::errno_exception("") );
	}
	pcImage->Load( pcStream );
	return pcImage;
}
