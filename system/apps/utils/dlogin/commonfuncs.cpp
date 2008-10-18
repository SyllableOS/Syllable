//  DLogin -:-  (C)opyright 2002-2005 Rick Caudill
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <string>
#include <posix/wait.h>

#include "commonfuncs.h"
#include "mainwindow.h"

/*********************************************
* Description: Returns the current time as a string
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetDateAndTime()
{
	DateTime d = DateTime::Now();
	return d.GetDate();
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
	delete( pcStream );
	return pcImage;
}

/*********************************************
* Description: Gets the system information
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetSystemInfo()
{
	String cReturn="";
	system_info sSysInfo;
	get_system_info(&sSysInfo);
	cReturn.Format("%d.%d.%d",( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) % 0xffff ), ( int )( sSysInfo.nKernelVersion & 0xffff));
	
	return cReturn;
}

IPoint GetResolution()
{
	Desktop cDesktop = Desktop();
	IPoint cPoint = cDesktop.GetResolution();
	return cPoint;
}

void UpdateLoginConfig(const String& cName)
{
	try
	{
		/*try to add the attribute "highlight" to dlogin*/
		File* pcFile = new File("/system/bin/dlogin",O_RDONLY);
		pcFile->RemoveAttr("highlight");
		if (pcFile->IsValid())
		{
			pcFile->WriteAttr("highlight",0, ATTR_TYPE_STRING,cName.c_str(),0,cName.size());
		}
		delete pcFile;
	}
	catch(...)
	{
	}
}

String GetHighlightName()
{
	try
	{
		char pzName[1024]="";

		/*try loading the highlight attribute from dlogin, then delete the file and return it*/
		File* pcFile = new File("/system/bin/dlogin",O_RDONLY);
		pcFile->ReadAttr("highlight", ATTR_TYPE_STRING,pzName,0,sizeof(pzName));
		delete pcFile;
		return pzName;
	}
	catch (...)
	{
		/*we hit a snag somewhere, so lets return a null string*/
		return ""; 
	}
}


BitmapImage* GetImageFromIcon(const String& cFile)
{
	/*user image file name*/
	String cIconFile = String("/system/icons/users/") + cFile + String(".png");
	
	/*returned image*/
	BitmapImage* pcReturnImage;
	
	try
	{
		/*if the file is valid then we will return the user image*/
		File* pcFile = new File(cIconFile,O_RDONLY);
		pcReturnImage = new BitmapImage(pcFile);
		delete( pcFile );
	}
	
	catch(...)
	{
		/*else we return the default image*/
		pcReturnImage = LoadImageFromResource("default.png");
	}
	
	return pcReturnImage;
}

int BecomeUser( struct passwd *psPwd )
{
	int nStatus;
    pid_t nError = waitpid(-1, &nStatus, WNOHANG);

    switch( (nError = fork()) )
    {
    case -1:
        break;

    case 0: /* child process */
		/*we are now going to become this user*/
		/*remember there is no security yet so this works fine*/
		/*later down the line we are probably going to have some work to do*/
        setuid( psPwd->pw_uid );
        setgid( psPwd->pw_gid );
        chdir( psPwd->pw_dir );
        setenv( "HOME", psPwd->pw_dir,true );
        setenv( "USER", psPwd->pw_name,true );
        setenv( "SHELL", psPwd->pw_shell,true );
        UpdateLoginConfig(psPwd->pw_name);
//		Application::GetInstance()->PopCursor();
        execl( "/system/bin/desktop", "desktop", NULL );
        break;


    default: /* parent process */
        int nDesktopPid, nExitStatus;
        nDesktopPid = nError;
        nError = waitpid( nDesktopPid, &nExitStatus, 0 );
       
        
        if( nError < 0 || nError != nDesktopPid ) // Something went wrong ;-)
            break;
            
        return 0;
    }
    return -errno;
}

