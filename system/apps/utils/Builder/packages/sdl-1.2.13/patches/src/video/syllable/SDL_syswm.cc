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

#include "SDL_Win.h"

extern "C" {

#include "SDL_syswm_c.h"
#include "SDL_error.h"

void SYL_SetWMCaption(_THIS, const char *title, const char *icon)
{
	SDL_Win->SetTitle(title);
}

int SYL_IconifyWindow(_THIS)
{
//	SDL_Win->Minimize(true);
	return 0;
}

int SYL_GetWMInfo(_THIS, SDL_SysWMinfo *info)
{
    if (info->version.major <= SDL_MAJOR_VERSION)
    {
        return 1;
    }
    else
    {
        SDL_SetError("Application not compiled with SDL %d.%d\n",
                      SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
        return -1;
    }
}

static inline os::color_space BitsPerPixelToColorSpace(int bpp)
{
	os::color_space colorspace;

	colorspace = os::CS_RGB16;
	switch (bpp) {
	    case 8:
		colorspace = os::CS_CMAP8;
		break;
   	    case 16:
		colorspace = os::CS_RGB16;
		break;
	    case 32:
		colorspace = os::CS_RGB32;
		break;
	    default:
		break;
	}
	return(colorspace);
}

void SYL_SetIcon(_THIS, SDL_Surface *icon, Uint8 *mask)
{
	if( icon == NULL || icon->format == NULL || ( icon->format->BitsPerPixel != 16 && icon->format->BitsPerPixel != 32 ) )
		return;
	os::Bitmap* pcBitmap = new os::Bitmap( icon->w, icon->h, BitsPerPixelToColorSpace( icon->format->BitsPerPixel ) );
	uint8* pPixels = pcBitmap->LockRaster();
	for( int i = 0; i < icon->h; i++ )
	{
		uint8* pPtr = pPixels + i * pcBitmap->GetBytesPerRow();
		memcpy( pPtr, (uint8*)icon->pixels + icon->pitch * i, icon->w * icon->format->BytesPerPixel );
	}
	pcBitmap->UnlockRaster();
	SDL_Win->SetIcon( pcBitmap );
	SDL_Win->Sync();
	delete( pcBitmap );
}

}; /* Extern C */




