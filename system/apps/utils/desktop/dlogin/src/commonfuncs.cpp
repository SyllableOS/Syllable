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
    char junk[1024];
    char login_info[1024];
    char login_name[1024];

    ifstream filRead;
    filRead.open("/boot/atheos/sys/config/login.cfg");

    filRead.getline(junk,1024);
    filRead.getline((char*)login_info,1024);
    filRead.close();

    FILE* fin;
    fin = fopen("/boot/atheos/sys/config/login.new","w");

    fprintf(fin,"<Login Name Option>\n");
    fprintf(fin, login_info);
    fprintf(fin, "\n<Login Name>\n");
    fprintf(fin,zName.c_str());
    fprintf(fin,"\n");
    fclose(fin);

    rename("/boot/atheos/sys/config/login.new", "/boot/atheos/sys/config/login.cfg");
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
    ifstream filRead;

    filRead.open("/boot/atheos/sys/config/login.cfg");
    filRead.getline(junk,1024);
    filRead.getline(login_info,1024);
    filRead.getline(junk, 1024);
    filRead.getline(junk,1024);
    filRead.close();

    if (strcmp(login_info,"true")==0)
    {
        return(junk);
    }
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
    if (filestr)
    {
        filestr.close();
        ReadLoginOption();
    }
}

os::String GetSyllableVersion()
{
    char zTemp[10], zReturn[100];

    FILE* fin = popen("/usr/bin/uname -vr 2>&1","r");
    fgets(zTemp, sizeof(zTemp),fin);
    pclose(fin);
	sprintf(zReturn,"Syllable v%c.%c.%c",zTemp[2],zTemp[4],zTemp[0]);
	return (os::String(zReturn));
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
