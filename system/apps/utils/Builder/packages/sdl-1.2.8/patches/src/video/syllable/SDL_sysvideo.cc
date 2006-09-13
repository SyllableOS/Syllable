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

/* Syllable based framebuffer implementation */

#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <unistd.h>
#include <gui/desktop.h>
#include <util/application.h>
#include <vector>

#include "SDL.h"
#include "SDL_SylApp.h"
#include "SDL_Win.h"
#include "SDL_timer.h"
#include "blank_cursor.h"

extern "C" {

#include "SDL_sysvideo.h"
#include "SDL_sysmouse_c.h"
#include "SDL_sysevents_c.h"
#include "SDL_events_c.h"
#include "SDL_syswm_c.h"
#include "SDL_lowvideo.h"
#include "SDL_yuvfuncs.h"
#include "SDL_pixels_c.h"

/* Initialization/Query functions */
static int SYL_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **SYL_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *SYL_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static void SYL_UpdateMouse(_THIS);
static int SYL_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void SYL_VideoQuit(_THIS);

/* Hardware surface functions */
static int SYL_AllocHWSurface(_THIS, SDL_Surface *surface);
static int SYL_LockHWSurface(_THIS, SDL_Surface *surface);
static void SYL_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void SYL_FreeHWSurface(_THIS, SDL_Surface *surface);

static int SYL_ToggleFullScreen(_THIS, int fullscreen);
#if 0
static SDL_Overlay *SYL_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display);
#endif

/* OpenGL functions */
#ifdef HAVE_OPENGL
static int BE_GL_LoadLibrary(_THIS, const char *path);
static void* BE_GL_GetProcAddress(_THIS, const char *proc);
static int BE_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value);
static int BE_GL_MakeCurrent(_THIS);
static void BE_GL_SwapBuffers(_THIS);
#endif
#define SYLLABLE_DEBUG
/* FB driver bootstrap functions */

static int SYL_Available(void)
{
	return(1);
}

static void SYL_DeleteDevice(SDL_VideoDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_VideoDevice *SYL_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *)malloc(sizeof(SDL_VideoDevice));
	if ( device ) {
		memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *)
				malloc((sizeof *device->hidden));
	}
	if ( (device == NULL) || (device->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( device ) {
			free(device);
		}
		return(0);
	}
	memset(device->hidden, 0, (sizeof *device->hidden));

	/* Set the function pointers */
	/* Initialization/Query functions */
	device->VideoInit = SYL_VideoInit;
	device->ListModes = SYL_ListModes;
	device->SetVideoMode = SYL_SetVideoMode;
	device->ToggleFullScreen = SYL_ToggleFullScreen;
	device->UpdateMouse = SYL_UpdateMouse;
	device->CreateYUVOverlay = NULL/*BE_CreateYUVOverlay*/;
	device->SetColors = SYL_SetColors;
	device->UpdateRects = NULL;
	device->VideoQuit = SYL_VideoQuit;
	/* Hardware acceleration functions */
	device->AllocHWSurface = SYL_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = SYL_LockHWSurface;
	device->UnlockHWSurface = SYL_UnlockHWSurface;
	device->FlipHWSurface = NULL;
	device->FreeHWSurface = SYL_FreeHWSurface;
	/* Gamma support */
#ifdef HAVE_OPENGL
	/* OpenGL support */
	device->GL_LoadLibrary = BE_GL_LoadLibrary;
	device->GL_GetProcAddress = BE_GL_GetProcAddress;
	device->GL_GetAttribute = BE_GL_GetAttribute;
	device->GL_MakeCurrent = BE_GL_MakeCurrent;
	device->GL_SwapBuffers = BE_GL_SwapBuffers;
#endif
	/* Window manager functions */
	device->SetCaption = SYL_SetWMCaption;
	device->SetIcon = NULL;
	device->IconifyWindow = SYL_IconifyWindow;
	device->GrabInput = NULL;
	device->GetWMInfo = SYL_GetWMInfo;
	/* Cursor manager functions */
	device->FreeWMCursor = SYL_FreeWMCursor;
	device->CreateWMCursor = SYL_CreateWMCursor;
	device->ShowWMCursor = SYL_ShowWMCursor;
	device->WarpWMCursor = SYL_WarpWMCursor;
	device->MoveWMCursor = NULL;
	device->CheckMouseMode = NULL;
	/* Event manager functions */
	device->InitOSKeymap = SYL_InitOSKeymap;
	device->PumpEvents = SYL_PumpEvents;

	device->free = SYL_DeleteDevice;

	/* Set the driver flags */
	device->handles_any_size = 1;
	
	return device;
}

VideoBootStrap SYLLABLE_Video_bootstrap = {
	"syllable", "Syllable graphics",
	SYL_Available, SYL_CreateDevice
};

static inline int ColorSpaceToBitsPerPixel(uint32 colorspace)
{
	int bitsperpixel;

	bitsperpixel = 0;
	switch (colorspace) {
//	    case os::CS_CMAP8:
//		bitsperpixel = 8;
//		break;
	    case os::CS_RGB15:
	    case os::CS_RGBA15:
		bitsperpixel = 15;
		break;
	    case os::CS_RGB16:
		bitsperpixel = 16;
		break;
	    case os::CS_RGB32:
	    case os::CS_RGBA32:
		bitsperpixel = 32;
		break;
	    default:
		break;
	}
	return(bitsperpixel);
}
static inline os::color_space BitsPerPixelToColorSpace(int bpp)
{
	os::color_space colorspace;

	colorspace = os::CS_RGB16;
	switch (bpp) {
//	    case 8:
//		colorspace = os::CS_CMAP8;
//		break;
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
#if 0
/* Function to sort the display_list in bscreen */
struct CompareModes
{
bool operator() ( const os::screen_mode a, const os::screen_mode b ) const
{
	printf( "%i %i\n", (int)b.m_nWidth, (int)a.m_nWidth );
	if ( a.m_eColorSpace == b.m_eColorSpace ) {
		printf("Same!\n");
		if( (b.m_nWidth*b.m_nHeight) > (a.m_nWidth*a.m_nHeight) )
			return( false );
		else if ( (b.m_nWidth*b.m_nHeight) < (a.m_nWidth*a.m_nHeight) )
			return( true );
		if( a.m_vRefreshRate < b.m_vRefreshRate )
			return( false );
		return( true );
	} else {
		return(ColorSpaceToBitsPerPixel(b.m_eColorSpace)-
		       ColorSpaceToBitsPerPixel(a.m_eColorSpace));
	}
}
};
#endif

/* Yes, this isn't the fastest it could be, but it works nicely */
static int SYL_AddMode(_THIS, int index, unsigned int w, unsigned int h, float r)
{
	SDL_Rect *mode;
	int i;
	int next_mode;

	/* Check to see if we already have this mode */
	if ( SDL_nummodes[index] > 0 ) {
		for ( i=SDL_nummodes[index]-1; i >= 0; --i ) {
			mode = SDL_modelist[index][i];
			if ( (mode->w == w) && (mode->h == h) ) {
#ifdef SYLLABLE_DEBUG
				fprintf(stderr, "We already have mode %dx%d at %d bytes per pixel\n", w, h, index+1);
#endif
				return(0);
			}
		}
	}

	/* Set up the new video mode rectangle */
	mode = (SDL_Rect *)malloc(sizeof *mode);
	if ( mode == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}
	mode->x = 0;
	mode->y = 0;
	mode->w = w;
	mode->h = h;
#ifdef SYLLABLE_DEBUG
	fprintf(stderr, "Adding mode %dx%d at %d bytes per pixel (%fHz)\n", w, h, index+1, r);
#endif

	/* Allocate the new list of modes, and fill in the new mode */
	next_mode = SDL_nummodes[index];
	SDL_modelist[index] = (SDL_Rect **)
	       realloc(SDL_modelist[index], (1+next_mode+1)*sizeof(SDL_Rect *));
	if ( SDL_modelist[index] == NULL ) {
		SDL_OutOfMemory();
		SDL_nummodes[index] = 0;
		free(mode);
		return(-1);
	}
	SDL_modelist[index][next_mode] = mode;
	SDL_modelist[index][next_mode+1] = NULL;
	SDL_nummodes[index]++;

	return(0);
}

int SYL_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	uint32 i, nmodes;
	int bpp;
	os::Rect bounds;
	std::vector<os::screen_mode> modes;
	modes.clear();

	/* Initialize the Syllable Application for appserver interaction */
	if ( SDL_InitSylApp() < 0 ) {
		return(-1);
	}


	/* It is important that this be created after SDL_InitSylApp() */
	os::Desktop screen;

	/* Save the current display mode */
	saved_mode = screen.GetScreenMode();
	/* Determine the screen depth */
	vformat->BitsPerPixel = ColorSpaceToBitsPerPixel(screen.GetColorSpace());
	if ( vformat->BitsPerPixel == 0 ) {
		SDL_SetError("Unknown screen colorspace: 0x%x",
						screen.GetColorSpace());
		return(-1);
	}

	/* Get the video modes we can switch to in fullscreen mode */
	nmodes = os::Application::GetInstance()->GetScreenModeCount();
	for( i = 0; i < nmodes; i++ )
	{
		os::screen_mode mode;
		os::Application::GetInstance()->GetScreenModeInfo( i, &mode );
		modes.push_back( mode );
	}

//	std::sort(modes.begin(), modes.end(), CompareModes() );

	for ( i=0; i<nmodes; ++i ) {
		bpp = ColorSpaceToBitsPerPixel(modes[i].m_eColorSpace);
		if ( bpp != 0 ) { // There are bugs in changing colorspace
		//if ( modes[i].m_eColorSpace == saved_mode.m_eColorSpace ) {
			SYL_AddMode(_this, ((bpp+7)/8)-1,
				modes[i].m_nWidth,
				modes[i].m_nHeight, modes[i].m_vRefreshRate);
		}
	}

	/* Create the window and view */
	bounds.top = 0; bounds.left = 0;
	bounds.right = 10;
	bounds.bottom = 10;
	SDL_Win = new SDL_SylWin(bounds);

#ifdef HAVE_OPENGL
	/* testgl application doesn't load library, just tries to load symbols */
	/* is it correct? if so we have to load library here */
	BE_GL_LoadLibrary(_this, NULL);
#endif

	/* Create the clear cursor */
	SDL_BlankCursor = SDL_CurrentCursor = SYL_CreateWMCursor(_this, blank_cdata, blank_cmask,
			BLANK_CWIDTH, BLANK_CHEIGHT, BLANK_CHOTX, BLANK_CHOTY);

	/* Fill in some window manager capabilities */
	_this->info.wm_available = 1;

	/* We're done! */
	return(0);
}

/* We support any dimension at our bit-depth */
SDL_Rect **SYL_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	SDL_Rect **modes;

	modes = ((SDL_Rect **)0);
	if ( (flags & SDL_FULLSCREEN) == SDL_FULLSCREEN ) {
		modes = SDL_modelist[((format->BitsPerPixel+7)/8)-1];
	} else {
		if ( format->BitsPerPixel ==
			_this->screen->format->BitsPerPixel ) {
			modes = ((SDL_Rect **)-1);
		}
		modes = ((SDL_Rect **)-1);
	}
	return(modes);
}

/* Various screen update functions available */
static void SYL_NormalUpdate(_THIS, int numrects, SDL_Rect *rects);


/* Find the closest display mode for fullscreen */
static bool SYL_FindClosestFSMode(_THIS, int width, int height, int bpp,
					 os::screen_mode *mode)
{
	os::Desktop screen;
	uint32 i, nmodes;
	os::screen_mode smode;
	os::screen_mode current;
	float current_refresh;
	current = screen.GetScreenMode();
	current_refresh = current.m_vRefreshRate;
	nmodes = os::Application::GetInstance()->GetScreenModeCount();
	for( i = nmodes - 1; i >= 0; i-- )
	{
		os::Application::GetInstance()->GetScreenModeInfo( i, &smode );
		if ( (bpp == ColorSpaceToBitsPerPixel(smode.m_eColorSpace)) &&
		     (width == smode.m_nWidth) &&
		     (height == smode.m_nHeight) ) {
			break;
		}
	}
	if ( i != 0 ) {
		*mode = smode;
		return true;
	} else {
		return false;
	}	
}

static int SYL_SetFullScreen(_THIS, SDL_Surface *screen, int fullscreen)
{

	int was_fullscreen;
	bool needs_unlock;
	os::Desktop dscreen;
	os::Rect bounds;
	os::screen_mode mode;
	int width, height, bpp;

	/* Set the fullscreen mode */

	was_fullscreen = SDL_Win->IsFullScreen();
	SDL_Win->SetFullScreen(fullscreen);
	fullscreen = SDL_Win->IsFullScreen();

	width = screen->w;
	height = screen->h;

	/* Set the appropriate video mode */

	if ( fullscreen ) {

		bpp = screen->format->BitsPerPixel;
		mode = dscreen.GetScreenMode();
		if ( (bpp != ColorSpaceToBitsPerPixel(mode.m_eColorSpace)) ||
		     (width != mode.m_nWidth) ||
		     (height != mode.m_nHeight)) {
			if(SYL_FindClosestFSMode(_this, width, height, bpp, &mode)) {
				mode.m_vRefreshRate = saved_mode.m_vRefreshRate;
				dscreen.SetScreenMode(&mode);
				/* This simply stops the next resize event from being
				 * sent to the SDL handler.
				 */
				SDL_Win->InhibitResize();
			} else {
				fullscreen = 0;
				SDL_Win->SetFullScreen(fullscreen);
			}
		}

	}
	if ( was_fullscreen && ! fullscreen ) {
		dscreen.SetScreenMode(&saved_mode);
	}


	if ( SDL_Win->Lock() == 0 ) {
		int xoff, yoff;
		if ( SDL_Win->Shown() ) {
			needs_unlock = 1;
			SDL_Win->Hide();
		} else {
			needs_unlock = 1;
		}
		/* This resizes the window and view area, but inhibits resizing
		 * of the BBitmap due to the InhibitResize call above. Thus the
		 * bitmap (pixel data) never changes.
		 */
		SDL_Win->ResizeTo(width-1, height-1);

		bounds = os::Rect( os::Point(), os::Point( dscreen.GetResolution() ) - os::Point( 1, 1 ) );
		/* Calculate offsets - used either to center window
		 * (windowed mode) or to set drawing offsets (fullscreen mode)
		 */
		xoff = ((int)bounds.Width() - width)/2;
		yoff = ((int)bounds.Height() - height)/2;
		SDL_Win->Show();
		SDL_Win->MakeFocus();
		if ( fullscreen ) {
			/* Set offset for drawing */
//			SDL_Win->SetXYOffset(xoff, yoff);
			SDL_Win->MoveTo( 0, 0 );		
		} else {
			/* Center window and reset the drawing offset */
			SDL_Win->SetXYOffset(0, 0);
			SDL_Win->CenterInScreen();
		}

		if ( ! needs_unlock || was_fullscreen ) {
			/* Center the window the first time */
			SDL_Win->CenterInScreen();

		}

		/* Unlock the window manually after the first Show() */
		if ( needs_unlock ) {
			SDL_Win->Unlock();
		}
	}

	/* Set the fullscreen flag in the screen surface */
	if ( fullscreen ) {
		screen->flags |= SDL_FULLSCREEN;
	} else {
		screen->flags &= ~SDL_FULLSCREEN; 
	}
	return(1);
}

static int SYL_ToggleFullScreen(_THIS, int fullscreen)
{
	return SYL_SetFullScreen(_this, _this->screen, fullscreen);
}

/* FIXME: check return values and cleanup here */
SDL_Surface *SYL_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	os::Desktop screen;
	os::Bitmap *bitmap;
	os::Rect bounds;

	Uint32 gl_flags = 0;
#if 0
	/* Only RGB works on r5 currently */
	gl_flags = BGL_RGB;
	if (_this->gl_config.double_buffer)
		gl_flags |= BGL_DOUBLE;
	else
		gl_flags |= BGL_SINGLE;
	if (_this->gl_config.alpha_size > 0 || bpp == 32)
		gl_flags |= BGL_ALPHA;
	if (_this->gl_config.depth_size > 0)
		gl_flags |= BGL_DEPTH;
	if (_this->gl_config.stencil_size > 0)
		gl_flags |= BGL_STENCIL;
	if (_this->gl_config.accum_red_size > 0
		|| _this->gl_config.accum_green_size > 0
		|| _this->gl_config.accum_blue_size > 0
		|| _this->gl_config.accum_alpha_size > 0)
		gl_flags |= BGL_ACCUM;
#endif
	/* Create the view for this window, using found flags */
	if ( SDL_Win->CreateView(flags, gl_flags) < 0 ) {
		return(NULL);
	}
	SDL_Win->View()->SetCursor( SDL_CurrentCursor );

	current->flags = SDL_ASYNCBLIT | SDL_PREALLOC;		/* Clear flags */
	current->w = width;
	current->h = height;
	if ( flags & SDL_NOFRAME ) {
		current->flags |= SDL_NOFRAME;
		SDL_Win->SetFlags(/*os::WND_SINGLEBUFFER|*/os::WND_NO_BORDER);
	} else {
		if ( (flags & SDL_RESIZABLE) && !(flags & SDL_OPENGL) )  {
			current->flags |= SDL_RESIZABLE;
			/* We don't want opaque resizing (TM). :-) */
			SDL_Win->SetFlags(0/*os::WND_SINGLEBUFFER*/);
		} else {
			SDL_Win->SetFlags(/*os::WND_SINGLEBUFFER|*/os::WND_NOT_RESIZABLE|os::WND_NO_ZOOM_BUT);
		}
	}

	if ( flags & SDL_OPENGL ) {
		current->flags |= SDL_OPENGL;
		current->pitch = 0;
		current->pixels = NULL;
		_this->UpdateRects = NULL;
	} else {
		/* Create the Bitmap framebuffer */
		os::color_space eSpace = BitsPerPixelToColorSpace( bpp );
		bitmap = new os::Bitmap( width, height, eSpace);
		current->pitch = bitmap->GetBytesPerRow();
		current->pixels = (void *)bitmap->LockRaster();
		SDL_Win->SetBitmap(bitmap);
		_this->UpdateRects = SYL_NormalUpdate;
		if ( ! SDL_ReallocFormat(current, ColorSpaceToBitsPerPixel( eSpace ), 0, 0, 0, 0) ) {
		return(NULL);
		}
	}
	/* Set the correct fullscreen mode */
	SYL_SetFullScreen(_this, current, flags & SDL_FULLSCREEN ? 1 : 0);

	/* We're done */
	return(current);
}

/* Update the current mouse state and position */
void SYL_UpdateMouse(_THIS)
{
	os::Point point;
	uint32 buttons;

	if ( SDL_Win->Lock() == 0 ) {
		/* Get new input state, if still active */
		if ( SDL_Win->IsActive() ) {
			(SDL_Win->View())->GetMouse(&point, &buttons);
		} else {
			point.x = -1;
			point.y = -1;
		}
		SDL_Win->Unlock();

		if ( (point.x >= 0) && (point.x < SDL_VideoSurface->w) &&
		     (point.y >= 0) && (point.y < SDL_VideoSurface->h) ) {
			SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
			SDL_PrivateMouseMotion(0, 0,
					(Sint16)point.x, (Sint16)point.y);
		} else {
			SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
		}
	}
}

/* We don't actually allow hardware surfaces other than the main one */
static int SYL_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	return(-1);
}
static void SYL_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}
static int SYL_LockHWSurface(_THIS, SDL_Surface *surface)
{
	return(0);
}
static void SYL_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	return;
}

static void SYL_NormalUpdate(_THIS, int numrects, SDL_Rect *rects)
{

	if ( SDL_Win->BeginDraw() ) {
		int i;

		for ( i=0; i<numrects; ++i ) {
			os::Rect rect;

			rect.top = rects[i].y;
			rect.left = rects[i].x;
			rect.bottom = rect.top+rects[i].h-1;
			rect.right = rect.left+rects[i].w-1;
			SDL_Win->DrawAsync(rect);
		}

		SDL_Win->EndDraw();
	}
}

#ifdef HAVE_OPENGL
/* Passing a NULL path means load pointers from the application */
int BE_GL_LoadLibrary(_THIS, const char *path)
{
	if (path == NULL) {
		if (_this->gl_config.dll_handle == NULL) {
			image_info info;
			int32 cookie = 0;
			while (get_next_image_info(0,&cookie,&info) == B_OK) {
				void *location = NULL;
				if (get_image_symbol((image_id)cookie,"glBegin",B_SYMBOL_TYPE_ANY,&location) == B_OK) {
					_this->gl_config.dll_handle = (void*)cookie;
					_this->gl_config.driver_loaded = 1;
					strncpy(_this->gl_config.driver_path, "libGL.so", sizeof(_this->gl_config.driver_path)-1);
				}
			}
		}
	} else {
		/*
			FIXME None of BeOS libGL.so implementations have exported functions 
			to load BGLView, which should be reloaded from new lib.
			So for now just "load" linked libGL.so :(
		*/
		if (_this->gl_config.dll_handle == NULL) {
			return BE_GL_LoadLibrary(_this, NULL);
		}

		/* Unload old first */
		/*if (_this->gl_config.dll_handle != NULL) {*/
			/* Do not try to unload application itself (if LoadLibrary was called before with NULL ;) */
		/*	image_info info;
			if (get_image_info((image_id)_this->gl_config.dll_handle, &info) == B_OK) {
				if (info.type != B_APP_IMAGE) {
					unload_add_on((image_id)_this->gl_config.dll_handle);
				}
			}
			
		}

		if ((_this->gl_config.dll_handle = (void*)load_add_on(path)) != (void*)B_ERROR) {
			_this->gl_config.driver_loaded = 1;
			strncpy(_this->gl_config.driver_path, path, sizeof(_this->gl_config.driver_path)-1);
		}*/
	}

	if (_this->gl_config.dll_handle != NULL) {
		return 0;
	} else {
		_this->gl_config.dll_handle = NULL;
		_this->gl_config.driver_loaded = 0;
		strcpy(_this->gl_config.driver_path, "");
		return -1;
	}
}

void* BE_GL_GetProcAddress(_THIS, const char *proc)
{
	if (_this->gl_config.dll_handle != NULL) {
		void *location = NULL;
		status_t err;
		if ((err = get_image_symbol((image_id)_this->gl_config.dll_handle, proc, B_SYMBOL_TYPE_ANY, &location)) == B_OK) {
			return location;
		} else {
			SDL_SetError("Couldn't find OpenGL symbol");
			return NULL;
		}
	} else {
		SDL_SetError("OpenGL library not loaded");
		return NULL;
	}
}

int BE_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value)
{
	/*
		FIXME? Right now BE_GL_GetAttribute shouldn't be called between glBegin() and glEnd() - it doesn't use "cached" values
	*/
	switch (attrib)
    {
		case SDL_GL_RED_SIZE:
			glGetIntegerv(GL_RED_BITS, (GLint*)value);
			break;
		case SDL_GL_GREEN_SIZE:
			glGetIntegerv(GL_GREEN_BITS, (GLint*)value);
			break;
		case SDL_GL_BLUE_SIZE:
			glGetIntegerv(GL_BLUE_BITS, (GLint*)value);
			break;
		case SDL_GL_ALPHA_SIZE:
			glGetIntegerv(GL_ALPHA_BITS, (GLint*)value);
			break;
		case SDL_GL_DOUBLEBUFFER:
			glGetBooleanv(GL_DOUBLEBUFFER, (GLboolean*)value);
			break;
		case SDL_GL_BUFFER_SIZE:
			int v;
			glGetIntegerv(GL_RED_BITS, (GLint*)&v);
			*value = v;
			glGetIntegerv(GL_GREEN_BITS, (GLint*)&v);
			*value += v;
			glGetIntegerv(GL_BLUE_BITS, (GLint*)&v);
			*value += v;
			glGetIntegerv(GL_ALPHA_BITS, (GLint*)&v);
			*value += v;
			break;
		case SDL_GL_DEPTH_SIZE:
			glGetIntegerv(GL_DEPTH_BITS, (GLint*)value); /* Mesa creates 16 only? r5 always 32 */
			break;
		case SDL_GL_STENCIL_SIZE:
			glGetIntegerv(GL_STENCIL_BITS, (GLint*)value);
			break;
		case SDL_GL_ACCUM_RED_SIZE:
			glGetIntegerv(GL_ACCUM_RED_BITS, (GLint*)value);
			break;
		case SDL_GL_ACCUM_GREEN_SIZE:
			glGetIntegerv(GL_ACCUM_GREEN_BITS, (GLint*)value);
			break;
		case SDL_GL_ACCUM_BLUE_SIZE:
			glGetIntegerv(GL_ACCUM_BLUE_BITS, (GLint*)value);
			break;
		case SDL_GL_ACCUM_ALPHA_SIZE:
			glGetIntegerv(GL_ACCUM_ALPHA_BITS, (GLint*)value);
			break;
		case SDL_GL_STEREO:
		case SDL_GL_MULTISAMPLEBUFFERS:
		case SDL_GL_MULTISAMPLESAMPLES:
		default:
			*value=0;
			return(-1);
	}
	return 0;
}

int BE_GL_MakeCurrent(_THIS)
{
	/* FIXME: should we glview->unlock and then glview->lock()? */
	return 0;
}

void BE_GL_SwapBuffers(_THIS)
{
	SDL_Win->SwapBuffers();
}
#endif

/* Is the system palette settable? */
int SYL_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
#if 0
	int i;
	SDL_Palette *palette;
	const color_map *cmap = BScreen().ColorMap();

	/* Get the screen colormap */
	palette = _this->screen->format->palette;
	for ( i=0; i<256; ++i ) {
		palette->colors[i].r = cmap->color_list[i].red;
		palette->colors[i].g = cmap->color_list[i].green;
		palette->colors[i].b = cmap->color_list[i].blue;
	}
#endif
	return(0);
}

void SYL_VideoQuit(_THIS)
{
	int i, j;

	SDL_Win->Quit();
	SDL_Win = NULL;

	if ( SDL_BlankCursor != NULL ) {
		SYL_FreeWMCursor(_this, SDL_BlankCursor);
		SDL_BlankCursor = NULL;
	}
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		if ( SDL_modelist[i] ) {
			for ( j=0; SDL_modelist[i][j]; ++j ) {
				free(SDL_modelist[i][j]);
			}
			free(SDL_modelist[i]);
			SDL_modelist[i] = NULL;
		}
	}
	/* Restore the original video mode */
	if ( _this->screen ) {
		if ( (_this->screen->flags&SDL_FULLSCREEN) == SDL_FULLSCREEN ) {
			os::Desktop screen;
			screen.SetScreenMode(&saved_mode);
		}
		_this->screen->pixels = NULL;
	}

#ifdef HAVE_OPENGL
	if (_this->gl_config.dll_handle != NULL)
		unload_add_on((image_id)_this->gl_config.dll_handle);
#endif

	SDL_QuitSylApp();
}

}; /* Extern C */
