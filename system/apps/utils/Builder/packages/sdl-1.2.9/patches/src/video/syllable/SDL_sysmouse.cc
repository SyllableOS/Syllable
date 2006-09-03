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

#include <stdlib.h>
#include <string.h>


#include "SDL_error.h"
#include "SDL_Win.h"

extern "C" {

#include "SDL_sysmouse_c.h"

/* Convert bits to padded bytes */
#define PADDED_BITS(bits)  ((bits+7)/8)


WMcursor *SYL_CreateWMCursor(_THIS,
		Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
	WMcursor *cursor;
	int allowed_x;
	int allowed_y;
	int run, pad, i, j, k;
	uint8 *cptr;

	allowed_x = 32;	/* Syllable limitation */
	allowed_y = 32;	/* Syllable limitation */
	if ( (w > allowed_x) || (h > allowed_y) ) {
		SDL_SetError("Only cursors of dimension (%dx%d) are allowed",
							allowed_x, allowed_y);
		return(NULL);
	}

	/* Allocate the cursor */
	cursor = (WMcursor *)malloc(sizeof(WMcursor));
	if ( cursor == NULL ) {
		SDL_OutOfMemory();
		return(NULL);
	}
	cursor->hotx = hot_x;
	cursor->hoty = hot_y;
	cursor->bits = (uint8 *)malloc(allowed_x*allowed_y);

	if ( cursor->bits == NULL ) {
		free(cursor);
		SDL_OutOfMemory();
		return(NULL);
	}
	memset( cursor->bits, 0, allowed_x*allowed_y );
	cptr = &cursor->bits[0];

	/* Pad out to the normal cursor size */
	run = PADDED_BITS(w);
	pad = allowed_x-w;
	for ( i=0; i<h; ++i ) {
		for( j=0; j < w / 8; j++ )
		{
			uint8 palette[2][2] = { { 0, 0 }, { 3, 2 } };
			for( k=7; k >= 0; k-- )
			{
				*cptr++ = palette[ ( *mask & ( 1 << k ) ) >> k][ ( *data & ( 1 << k ) ) >> k ];
			}
			mask++;
			data++;
		}
		cptr += pad;
	}
	for ( ; i<allowed_y; ++i ) {
		memset(cptr, 0, allowed_x);
		cptr += (allowed_x);
	}

	return(cursor);
}

int SYL_ShowWMCursor(_THIS, WMcursor *cursor)
{

	//if ( os::Application::GetInstance()->Lock() == 0 ) {
		if ( cursor == NULL ) {
			if ( SDL_BlankCursor != NULL ) {
				SDL_CurrentCursor = SDL_BlankCursor;
				if( SDL_Win->View() )
					SDL_Win->View()->SetCursor( SDL_BlankCursor );
			}
		} else {
			SDL_CurrentCursor = cursor;
			if( SDL_Win->View() )
				SDL_Win->View()->SetCursor( cursor );

		}
//		os::Application::GetInstance()->Unlock();
//	}

	return(1);
}

void SYL_FreeWMCursor(_THIS, WMcursor *cursor)
{
	free(cursor->bits);
	free(cursor);
}


void SYL_WarpWMCursor(_THIS, Uint16 x, Uint16 y)
{
	os::Point pt(x, y);
	SDL_Win->Lock();
	SDL_Win->View()->SetMousePos( os::Point( x, y ) );
	SDL_Win->Unlock();
}

}; /* Extern C */
