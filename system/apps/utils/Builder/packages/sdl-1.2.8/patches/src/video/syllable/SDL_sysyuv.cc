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

/* This is the Syllable version of SDL YUV video overlays */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_sysyuv.h"
#include "SDL_yuvfuncs.h"

extern "C" {

/* The functions used to manipulate software video overlays */
static struct private_yuvhwfuncs syl_yuvfuncs =
{
    SYL_LockYUVOverlay,
    SYL_UnlockYUVOverlay,
    SYL_DisplayYUVOverlay,
    SYL_FreeYUVOverlay
};



SDL_Overlay *SYL_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display) {
	SDL_Overlay* overlay;
	struct private_yuvhwdata* hwdata;

	/* Check format */
	if( format != SDL_YV12_OVERLAY )
		return( NULL );

	//printf("Create!\n" );

    /* Create the overlay structure */
    overlay = (SDL_Overlay*)calloc(1, sizeof(SDL_Overlay));

    if (overlay == NULL)
    {
        SDL_OutOfMemory();
        return NULL;
    }

	/* Calculate size */

	uint32 nDstPitch = ( ( width ) + 0x1ff ) & ~0x1ff;
	int nSize = nDstPitch * height * 2;

	//printf("%i %i bytes\n", nSize, (int)nDstPitch );

	/* Get media manager */

	os::MediaManager* pcManager = NULL;
	try
	{
		pcManager = os::MediaManager::Get();
	}
	catch( ... )
	{
		printf("Could not get media manager!\n" );
		SDL_FreeYUVOverlay(overlay);
		return( NULL );
	}

	/* Get media output */
	os::MediaOutput* pcOutput = pcManager->GetDefaultVideoOutput();
	if( pcOutput == NULL || pcOutput->FileNameRequired() || ( pcOutput->Open( "" ) != 0 ) )
	{
		printf("Could not get output!\n" );
		SDL_FreeYUVOverlay(overlay);
		return( NULL );
	}

	/* Add stream */
	os::MediaFormat_s sFormat;
	MEDIA_CLEAR_FORMAT( sFormat );
	sFormat.nType = os::MEDIA_TYPE_VIDEO;
	sFormat.zName = "Raw Video";
	sFormat.nWidth = width;
	sFormat.nHeight = height;
	sFormat.eColorSpace = os::CS_YUV12;

	if( pcOutput->AddStream( "SDL Overlay", sFormat ) != 0 )
	{
		printf("Could not get create overlay!\n" );
		SDL_FreeYUVOverlay(overlay);
		return( NULL );
	}

    /* Fill in the basic members */
    overlay->format = format;
    overlay->w = width;
    overlay->h = height;
    overlay->hwdata = NULL;
	
    /* Set up the YUV surface function structure */
    overlay->hwfuncs = &syl_yuvfuncs;

    /* Create the pixel data and lookup tables */
    hwdata = (struct private_yuvhwdata*)calloc(1, sizeof(struct private_yuvhwdata));

    if (hwdata == NULL)
    {
        SDL_OutOfMemory();
        SDL_FreeYUVOverlay(overlay);
        return NULL;
    }

	/* Allocate buffer */
	uint8* pBuffer = (uint8*)malloc( nSize );
	if( pBuffer == NULL )
	{
		SDL_OutOfMemory();
		SDL_FreeYUVOverlay(overlay);
		return NULL;
	}

    overlay->hwdata = hwdata;
	overlay->hwdata->pcManager = pcManager;
	overlay->hwdata->pBuffer = pBuffer;
	overlay->hwdata->pcOutput = pcOutput;
	overlay->hwdata->locked = 0;
	overlay->hwdata->pcView = pcOutput->GetVideoView( 0 );
	overlay->hwdata->first_display = true;
	if( overlay->hwdata->pcView )
		overlay->hwdata->pcView->Hide();
	
	overlay->planes = 3;
	overlay->pitches = (Uint16*)calloc(3, sizeof(Uint16));
	overlay->pixels  = (Uint8**)calloc(3, sizeof(Uint8*));
	if (!overlay->pitches || !overlay->pixels)
	{
        SDL_OutOfMemory();
        SDL_FreeYUVOverlay(overlay);
        return(NULL);
    }

	overlay->pitches[0] = nDstPitch;
	overlay->pitches[1] = nDstPitch / 2;
	overlay->pitches[2] = nDstPitch / 2;
	
	overlay->pixels[0]  = overlay->hwdata->pBuffer;
	overlay->pixels[1]  = overlay->hwdata->pBuffer + height * nDstPitch * 5 / 4;
	overlay->pixels[2]  = overlay->hwdata->pBuffer + height * nDstPitch;

	overlay->hw_overlay = 1;
	
	if (SDL_Win->Lock() != 0) {
		printf("Could not lock window!\n" );
        SDL_FreeYUVOverlay(overlay);
        return(NULL);
    }
	os::View * view = SDL_Win->View();
	if( overlay->hwdata->pcView )
	    view->AddChild(overlay->hwdata->pcView);
	SDL_Win->Unlock();
	
	current_overlay=overlay;

        
	return overlay;
}

int SYL_LockYUVOverlay(_THIS, SDL_Overlay* overlay)
{
    if (overlay == NULL)
    {
        return 0;
    }

    overlay->hwdata->locked = 1;
    return 0;
}

void SYL_UnlockYUVOverlay(_THIS, SDL_Overlay* overlay)
{
    if (overlay == NULL)
    {
         return;
    }

    overlay->hwdata->locked = 0;
}

int SYL_DisplayYUVOverlay(_THIS, SDL_Overlay* overlay, SDL_Rect* dstrect)
{
	//printf("Display!\n" );
    if ((overlay == NULL) || (overlay->hwdata==NULL)
        || (overlay->hwdata->pcOutput==NULL) || (SDL_Win->View() == NULL))
    {
        return -1;
    }

	os::MediaPacket_s sPacket;
	sPacket.nStream = 0;
	sPacket.nTimeStamp = ~0;
	sPacket.pBuffer[0] = overlay->pixels[0];
	sPacket.pBuffer[2] = overlay->pixels[1];
	sPacket.pBuffer[1] = overlay->pixels[2];
	sPacket.nSize[0] = overlay->pitches[0];
	sPacket.nSize[1] = overlay->pitches[1];
	sPacket.nSize[2] = overlay->pitches[2];

	overlay->hwdata->pcOutput->WritePacket( 0, &sPacket );

    if (SDL_Win->Lock() != 0) {
        return 0;
    }
    os::View* pcView = overlay->hwdata->pcView;
	if( pcView )
	{
	    if (SDL_Win->IsFullScreen()) {
    		int left,top;
	    	SDL_Win->GetXYOffset(left,top);
		    pcView->MoveTo(left+dstrect->x,top+dstrect->y);
	    } else {
		    pcView->MoveTo(dstrect->x,dstrect->y);
    	}
	    pcView->ResizeTo(dstrect->w-1,dstrect->h-1);
    	pcView->Flush();
		if (overlay->hwdata->first_display) {
			//printf("Show overlay!\n" );
			pcView->Show();
			overlay->hwdata->first_display = false;
		}
	}
    SDL_Win->Unlock();
	return 0;
}

void SYL_FreeYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	//printf("SYL_FreeYUVOverlay\n");
    if (overlay == NULL)
    {
        return;
    }

    if (overlay->hwdata == NULL)
    {
        return;
    }

	if( overlay->hwdata->pcView )
		overlay->hwdata->pcView->RemoveThis();
	overlay->hwdata->pcOutput->Close();
	overlay->hwdata->pcOutput->Release();
	overlay->hwdata->pcManager->Put();

    current_overlay=NULL;

    free(overlay->hwdata);
}

}; // extern "C"











