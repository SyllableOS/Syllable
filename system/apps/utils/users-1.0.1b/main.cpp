/*
 *  Users and Passwords Manager for AtheOS
 *  Copyright (C) 2002 William Rose
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <util/application.h>

#include "userswindow.h"
#include "main.h"
#include <atheos/filesystem.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <crypt.h>
#include <gui/image.h>
#include <util/resources.h>
#include <gui/requesters.h>

using namespace os;

int main( int argc, char **argv ) {
  Application *pcApp = new Application( "application/x-vnd.WDR-user_prefs" );
  UsersWindow *pcMain = new UsersWindow( Rect( 0, 0, 450 - 1, 400 - 1 ));
  
  pcMain->CenterInScreen();
  pcMain->Show();
  pcMain->MakeFocus( true );

  pcApp->Run();
}

BitmapImage* LoadImage(const std::string& zResource)
{
    Resources cCol(get_image_id());
    ResStream* pcStream = cCol.GetResourceStream(zResource);
    BitmapImage* vImage = new BitmapImage(pcStream);
    return(vImage);
}

bool compare_pwd_to_pwd( const char *pzCmp, const char *pzPwd ) {
  return strcmp( pzCmp, pzPwd );
}

bool compare_pwd_to_txt( const char *pzCmp, const char *pzTxt ) {
  return strcmp( pzCmp, crypt( pzTxt, "$1$" ) ) == 0;
}



void errno_alert( const char *pzTitle, const char *pzAction, Window* pcWin=NULL ) {
  char acMsg[256];
  
  snprintf( acMsg, 256, "An error occurred while %s (%d: %s).",
            pzAction, errno, strerror( errno ) );
  
  Alert *pcAlert = new Alert( pzTitle, acMsg, Alert::ALERT_WARNING, 0, "OK", NULL );

  if (pcWin != NULL)
  	pcAlert->CenterInWindow(pcWin);
  pcAlert->Go(new Invoker());
}


