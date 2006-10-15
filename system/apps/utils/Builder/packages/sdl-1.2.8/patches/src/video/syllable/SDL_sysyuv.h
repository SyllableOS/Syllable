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


#ifndef __SDL_SYS_YUV_H__
#define __SDL_SYS_YUV_H__

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif /* SAVE_RCSID */

/* This is the Syllable implementation of YUV video overlays */

#include "SDL_video.h"
#include "SDL_lowvideo.h"

#include <media/manager.h>
#include <media/format.h>
#include <media/output.h>

extern "C" {

struct private_yuvhwdata
{
	uint8* pBuffer;
	os::MediaManager* pcManager;
	os::MediaOutput* pcOutput;
	os::View* pcView;
	int first_display;
    int locked;
};

extern SDL_Overlay* SYL_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface* display);
extern int SYL_LockYUVOverlay(_THIS, SDL_Overlay* overlay);
extern void SYL_UnlockYUVOverlay(_THIS, SDL_Overlay* overlay);
extern int SYL_DisplayYUVOverlay(_THIS, SDL_Overlay* overlay, SDL_Rect* dstrect);
extern void SYL_FreeYUVOverlay(_THIS, SDL_Overlay* overlay);

};

#endif /* __SDL_PH_YUV_H__ */


