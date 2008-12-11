/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Handle the Syllable specific portions of the application */

#include <util/application.h>
#include <atheos/filesystem.h>
#include <atheos/image.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "SDL_SylApp.h"
#include "SDL_thread.h"
#include "SDL_timer.h"
#include "SDL_error.h"

/* Flag to tell whether or not the application is active or not */
int SDL_SylAppActive = 0;
static SDL_Thread *SDL_AppThread = NULL;

static int StartSylApp(void *unused)
{
	os::Application *App;

	App = new os::Application("application/x-SDL-executable");

	App->Run();
	return(0);
}

/* Initialize the Application, if it's not already started */
int SDL_InitSylApp(void)
{
	/* Create the BApplication that handles appserver interaction */
	if ( SDL_SylAppActive <= 0 ) {
		SDL_AppThread = SDL_CreateThread(StartSylApp, NULL);
		if ( SDL_AppThread == NULL ) {
			SDL_SetError("Couldn't create Application thread");
			return(-1);
		}
		
		/* Change working to directory to that of executable */
		int nDir = open_image_file( IMGFILE_BIN_DIR );
		char zBuffer[PATH_MAX];
		get_directory_path( nDir, zBuffer, PATH_MAX );
		chdir( zBuffer );
		printf("%s\n", zBuffer );
		close( nDir );
		do {
			SDL_Delay(10);
		} while ( os::Application::GetInstance() == NULL );

		/* Mark the application active */
		SDL_SylAppActive = 0;
	}

	/* Increment the application reference count */
	++SDL_SylAppActive;

	/* The app is running, and we're ready to go */
	return(0);
}

/* Quit the Application, if there's nothing left to do */
void SDL_QuitSylApp(void)
{
	/* Decrement the application reference count */
	--SDL_SylAppActive;

	/* If the reference count reached zero, clean up the app */
	if ( SDL_SylAppActive == 0 ) {
		if ( SDL_AppThread != NULL ) {
			if ( os::Application::GetInstance() != NULL ) { /* Not tested */
				os::Application::GetInstance()->PostMessage(os::M_QUIT);
			}
			SDL_WaitThread(SDL_AppThread, NULL);
			SDL_AppThread = NULL;
		}
	}
}
