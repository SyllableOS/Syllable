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

#ifndef _SDL_BWin_h
#define _SDL_BWin_h

#include <stdio.h>
#include <gui/window.h>
#include <gui/view.h>
#include <gui/bitmap.h>

#include "SDL_SylApp.h"
#include "SDL_events.h"
#include "SDL_View.h"
#include "SDL_error.h"

extern "C" {
#include "SDL_events_c.h"
};

class SDL_SylWin : public os::Window
{
public:
	SDL_SylWin(os::Rect bounds) :
			os::Window(bounds, "sdl", "SDL", 0/*os::WND_SINGLEBUFFER*/) {
		fullscreen = false;

		the_view = NULL;
#ifdef HAVE_OPENGL
		SDL_GLView = NULL;
#endif
		SDL_View = NULL;
		shown = false;
		inhibit_resize = false;
	}

	virtual ~SDL_SylWin() {
//		Lock();
		if ( the_view ) {
#ifdef HAVE_OPENGL
			if ( the_view == SDL_GLView ) {
				SDL_GLView->UnlockGL();
			}
#endif
			RemoveChild(the_view);
			the_view = NULL;
		}
//		Unlock();
#ifdef HAVE_OPENGL
		if ( SDL_GLView ) {
			delete SDL_GLView;
		}
#endif
		if ( SDL_View ) {
			delete SDL_View;
		}
	}
	bool IsFullScreen()
	{
		return( fullscreen );
	}
	void SetFullScreen( bool f )
	{
		fullscreen = f;
		if( fullscreen )
			MoveTo( 0, 0 );
		else
			CenterInScreen();
	}
	/* Override the Show() method so we can tell when we've been shown */
	virtual void Show(void) {
		Lock();
		os::Window::Show();
		Unlock();
		shown = true;
	}
	virtual bool Shown(void) {
		return (shown);
	}
	/* If called, the next resize event will not be forwarded to SDL. */
	virtual void InhibitResize(void) {
		inhibit_resize=true;
	}
	/* Handle resizing of the window */
	virtual void FrameSized(const os::Point& cDelta) {
		if(inhibit_resize)
			inhibit_resize = false;
		else 
			SDL_PrivateResize((int)GetBounds().Width() + 1, (int)GetBounds().Height() + 1);
	}
	virtual int CreateView(Uint32 flags, Uint32 gl_flags) {
		int retval;

		retval = 0;
		Lock();
		if ( flags & SDL_OPENGL ) {
#ifdef HAVE_OPENGL
			if ( SDL_GLView == NULL ) {
				SDL_GLView = new BGLView(Bounds(), "SDL GLView",
					 	B_FOLLOW_ALL_SIDES, (B_WILL_DRAW|B_FRAME_EVENTS),
					 	gl_flags);
			}
			if ( the_view != SDL_GLView ) {
				if ( the_view ) {
					RemoveChild(the_view);
				}
				AddChild(SDL_GLView);
				SDL_GLView->LockGL();
				the_view = SDL_GLView;
			}
#else
			SDL_SetError("OpenGL support not enabled");
			retval = -1;
#endif
		} else {
			if ( SDL_View == NULL ) {
				SDL_View = new SDL_SylView(GetBounds());
			}
			if ( the_view != SDL_View ) {
				if ( the_view ) {
#ifdef HAVE_OPENGL
					if ( the_view == SDL_GLView ) {
						SDL_GLView->UnlockGL();
					}
#endif
					RemoveChild(the_view);
				}
				AddChild(SDL_View);
				SetFocusChild(SDL_View);
				the_view = SDL_View;
			}
		}
		Unlock();
		return(retval);
	}
	virtual void SetBitmap(os::Bitmap *bitmap) {
		SDL_View->SetBitmap(bitmap);
	}
	virtual void SetXYOffset(int x, int y) {
#ifdef HAVE_OPENGL
		if ( the_view == SDL_GLView ) {
			return;
		}
#endif
		SDL_View->SetXYOffset(x, y);		
	}
	virtual void GetXYOffset(int &x, int &y) {
#ifdef HAVE_OPENGL
		if ( the_view == SDL_GLView ) {
			x = 0;
			y = 0;
			return;
		}
#endif
		SDL_View->GetXYOffset(x, y);
	}
	virtual bool BeginDraw(void) {
//		return(Lock()==0);
		return( 1 );
	}
	virtual void DrawAsync(os::Rect updateRect) {
		SDL_View->DrawAsync(updateRect);
	}
	virtual void EndDraw(void) {
		SDL_View->Flush();
//		Unlock();
	}
#ifdef HAVE_OPENGL
	virtual void SwapBuffers(void) {
		SDL_GLView->UnlockGL();
		SDL_GLView->LockGL();
		SDL_GLView->SwapBuffers();
	}
#endif
	virtual SDL_SylView *View(void) {
		return(SDL_View);
	}

	/* Hook functions -- overridden */
	virtual void Minimize(bool minimize) {
		/* This is only called when mimimized, not when restored */
		//SDL_PrivateAppActive(minimize, SDL_APPACTIVE);
//		BWindow::Minimize(minimize);
	}
	virtual void WindowActivated(bool active) {
		SDL_PrivateAppActive(active, SDL_APPINPUTFOCUS);
	}
	virtual bool OkToQuit(void) {
		if ( SDL_SylAppActive > 0 ) {
			SDL_PrivateQuit();
			/* We don't ever actually close the window here because
			   the application should respond to the quit request,
			   or ignore it as desired.
			 */
			return(false);
		}
		return(true);	/* Close the app window */
	}
private:
#ifdef HAVE_OPENGL
	BGLView *SDL_GLView;
#endif
	SDL_SylView *SDL_View;
	os::View *the_view;

	bool shown;
	bool inhibit_resize;
	bool fullscreen;

	int32 last_buttons;
	SDLKey keymap[128];
};

#endif /* _SDL_SylWin_h */
