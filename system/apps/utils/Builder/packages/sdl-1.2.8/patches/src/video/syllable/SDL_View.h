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

#ifndef _SDL_BView_h
#define _SDL_BView_h

#include <gui/font.h>
#include <util/application.h>
#include <util/message.h>

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* This is the event handling and graphics update portion of SDL_BWin */

extern "C" {
#include "SDL_events_c.h"


/* The implementation dependent data for the window manager cursor */
struct WMcursor {
	int hotx, hoty;
	uint8 *bits;
};

};

class SDL_SylView : public os::View
{
public:
	SDL_SylView(os::Rect frame) : 
		os::View(frame, "SDL View", os::CF_FOLLOW_ALL,
					(os::WID_WILL_DRAW)) {
		InitKeyboard();
		image = NULL;
		xoff = yoff = 0;
		cursor = false;
		last_qual = 0;
		SetFgColor(0,0,0,0);
		SetBgColor(0,0,0,0);
	}
	virtual ~SDL_SylView() {
		SetBitmap(NULL);
	}
	/* Set drawing offsets for fullscreen mode */
	virtual void SetXYOffset(int x, int y) {
		xoff = x;
		yoff = y;
	}
	virtual void GetXYOffset(int &x, int &y) {
		x = xoff;
		y = yoff;
	}
	/* The view changed size. If it means we're in fullscreen, we
	 * draw a nice black box in the entire view to get black borders.
	 */
	virtual void FrameResized(float width, float height) {
		os::Rect bounds = GetBounds();
		/* Fill the entire view with black */ 
		FillRect(bounds);
		/* And if there's an image, redraw it. */
		if( image ) {
			//bounds = image->Bounds();
			Invalidate();
			Flush();
		}
	}
	
	/* Drawing portion of this complete breakfast. :) */
	virtual void SetBitmap(os::Bitmap *bitmap) {
		if ( image ) {
			delete image;
		}
		image = bitmap;
	}
	virtual void Paint( const os::Rect& cUpdateRect ) {
		if ( image ) {
			os::Rect updateRect = cUpdateRect;
			if(xoff || yoff) {	
				os::Rect dest;
				dest.top    = updateRect.top + yoff;
				dest.left   = updateRect.left + xoff;
				dest.bottom = updateRect.bottom + yoff;
				dest.right  = updateRect.right + xoff;
				DrawBitmap(image, updateRect, dest);
			} else {
				DrawBitmap(image, updateRect & image->GetBounds(), updateRect & image->GetBounds() );
			}
		}
	}
	virtual void DrawAsync(const os::Rect& cUpdateRect) {
		if ( image ) {
			os::Rect updateRect = cUpdateRect;
			if(xoff || yoff) {	
				os::Rect dest;
				dest.top    = updateRect.top + yoff;
				dest.left   = updateRect.left + xoff;
				dest.bottom = updateRect.bottom + yoff;
				dest.right  = updateRect.right + xoff;
				DrawBitmap(image, updateRect, dest);
			} else {
				DrawBitmap(image, updateRect & image->GetBounds(), updateRect & image->GetBounds() );
			}
		}

	}
	
	virtual void Activated(bool active) {
		SDL_PrivateAppActive(active, SDL_APPINPUTFOCUS);
	}
	virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData )
	{
		if( nCode == os::MOUSE_ENTERED )
		{
			os::Application::GetInstance()->PushCursor( os::MPTR_MONO, CurrentCursor->bits, 
				32, 32, os::IPoint( CurrentCursor->hotx, CurrentCursor->hoty ) );
			cursor = true;
		}
		if( nCode == os::MOUSE_EXITED )
		{
			os::Application::GetInstance()->PopCursor();
			if ( SDL_GetAppState() & SDL_APPMOUSEFOCUS ) {
				SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
			}
			cursor = false;
		} 
		if( nCode != os::MOUSE_EXITED && nCode != os::MOUSE_OUTSIDE ) {
			
			int x, y;
			if ( ! (SDL_GetAppState() & SDL_APPMOUSEFOCUS) ) {
				SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
				//SDL_SetCursor(NULL);
			}
			GetXYOffset(x, y);
			x = (int)cNewPos.x - x;
			y = (int)cNewPos.y - y;
			SDL_PrivateMouseMotion(0, 0, x, y);
		}
		os::View::MouseMove( cNewPos, nButtons, nCode, pcData );
	}
    virtual void MouseDown( const os::Point& cPosition, uint32 buttons )
    {
		int sdl_buttons = 0;

		/* Add any mouse button events */
		if (buttons == 1) {
			sdl_buttons |= SDL_BUTTON_LEFT;
		}
		if (buttons == 2) {
			sdl_buttons |= SDL_BUTTON_RIGHT;
		}
		if (buttons == 3) {
			sdl_buttons |= SDL_BUTTON_MIDDLE;
		}
		SDL_PrivateMouseButton(SDL_PRESSED, sdl_buttons, 0, 0);
    	os::View::MouseDown( cPosition, buttons );
    }
    virtual void MouseUp( const os::Point& cPosition, uint32 buttons, os::Message* pcData )
    {
    	int sdl_buttons = 0;

		/* Add any mouse button events */
		if (buttons == 1) {
			sdl_buttons |= SDL_BUTTON_LEFT;
		}
		if (buttons == 2) {
			sdl_buttons |= SDL_BUTTON_RIGHT;
		}
		if (buttons == 3) {
			sdl_buttons |= SDL_BUTTON_MIDDLE;
		}
		SDL_PrivateMouseButton(SDL_RELEASED, sdl_buttons, 0, 0);

    	os::View::MouseUp( cPosition, buttons, pcData );
    }    
    virtual int16 Translate2Unicode(const char *buf) {
		Uint16 unicode = 0;

		if ((uchar)buf[0] > 127) {
			unicode = os::utf8_to_unicode( buf );
		} else
			unicode = (Uint16)buf[0];

		return unicode;
	}
	virtual void InitKeyboard(void) {
		for ( uint i=0; i<SDL_TABLESIZE(keymap); ++i )
			keymap[i] = (SDLKey)i;

		keymap[os::VK_BACKSPACE] = SDLK_BACKSPACE;
		keymap[os::VK_ENTER] = SDLK_RETURN;
		keymap[os::VK_SPACE] = SDLK_SPACE;
		keymap[os::VK_TAB] = SDLK_TAB;
		keymap[os::VK_ESCAPE] = SDLK_ESCAPE;
		keymap[os::VK_LEFT_ARROW] = SDLK_LEFT;
		keymap[os::VK_RIGHT_ARROW] = SDLK_RIGHT;
		keymap[os::VK_UP_ARROW] = SDLK_UP;
		keymap[os::VK_DOWN_ARROW] = SDLK_DOWN;
		keymap[os::VK_INSERT] = SDLK_INSERT;
		keymap[os::VK_DELETE] = SDLK_DELETE;
		keymap[os::VK_HOME] = SDLK_HOME;
		keymap[os::VK_END] = SDLK_END;
		keymap[os::VK_PAGE_UP] = SDLK_PAGEUP;
		keymap[os::VK_PAGE_DOWN] = SDLK_PAGEDOWN;
	}    
	inline SDLKey ConvertQualifiers( uint32 nQual )
	{
		switch( nQual )
		{
			case os::QUAL_LSHIFT:
				return( SDLK_LSHIFT );
			break;
			case os::QUAL_RSHIFT:
				return( SDLK_RSHIFT );
			break;
			case os::QUAL_LCTRL:
				return( SDLK_LCTRL );
			break;
			case os::QUAL_RCTRL:
				return( SDLK_RCTRL );
			break;
			case os::QUAL_LALT:
				return( SDLK_LALT );
			break;
			case os::QUAL_RALT:
				return( SDLK_RALT );
			break;
		}
		return( SDLK_UNKNOWN );
	}
	inline void CheckQualifiers( uint32 nQualifiers, bool bPressed )
	{
		static uint32 anQualifierList[] =
		{
			os::QUAL_LSHIFT,
			os::QUAL_RSHIFT,
			os::QUAL_LCTRL,
			os::QUAL_RCTRL,
			os::QUAL_LALT,
			os::QUAL_RALT
		};
		
		for( uint i = 0; i < sizeof( anQualifierList ) / 4; i++ )
		{
			if( nQualifiers & anQualifierList[i] )
			{
				SDL_keysym keysym;
				keysym.scancode = 0;
				keysym.sym = ConvertQualifiers( anQualifierList[i] );
				keysym.mod = KMOD_NONE;
				keysym.unicode = 0;
				SDL_PrivateKeyboard( bPressed ? SDL_PRESSED : SDL_RELEASED, &keysym);
			}
		}
	}
	virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
    {
    	SDL_keysym keysym;
    	if( nQualifiers & os::QUAL_REPEAT )
    		return( os::View::KeyDown( pzString, pzRawString, nQualifiers ) );
		keysym.scancode = pzRawString[0];
		if( pzRawString[0] == os::VK_FUNCTION_KEY )
		{
			os::Message* pcKey = GetLooper()->GetCurrentMessage();
			int32 nRawKey;
			pcKey->FindInt32("_raw_key", &nRawKey);
			keysym.sym = (SDLKey)(SDLK_F1 + nRawKey - 2);
		}
		else if( pzRawString[0] == 0 && nQualifiers != 0  )
		{
			/* Compare to last qualifiers */
			uint32 nNewQualifiers = nQualifiers ^ last_qual;
			nNewQualifiers &= nQualifiers;
			CheckQualifiers( nNewQualifiers, true );
			last_qual = nQualifiers;
			return( os::View::KeyDown( pzString, pzRawString, nQualifiers ) );
		}
		else if( (uint8)pzRawString[0] < 128 )
			keysym.sym = keymap[(uint8)pzRawString[0]];
		else 
			keysym.sym = SDLK_UNKNOWN;

		keysym.mod = KMOD_NONE;
		keysym.unicode = 0;
		if (SDL_TranslateUNICODE)	
			keysym.unicode = Translate2Unicode( pzRawString );
		SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
		
    	//printf("Down %x %x %i\n", (uint)pzRawString[0], (uint)nQualifiers, keysym.sym );
    	os::View::KeyDown( pzString, pzRawString, nQualifiers );
    }
    virtual void KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers )
	{
		SDL_keysym keysym;
		if( nQualifiers & os::QUAL_REPEAT )
    		return( os::View::KeyDown( pzString, pzRawString, nQualifiers ) );

		keysym.scancode = pzRawString[0];
		if( pzRawString[0] == os::VK_FUNCTION_KEY )
		{
			os::Message* pcKey = GetLooper()->GetCurrentMessage();
			int32 nRawKey;
			pcKey->FindInt32("_raw_key", &nRawKey);
			keysym.sym = (SDLKey)(SDLK_F1 + nRawKey - 130);
		}
		else if( pzRawString[0] == 0 ) {
			/* Compare to last qualifiers */
			uint32 nNewQualifiers = nQualifiers ^ last_qual;
			nNewQualifiers &= last_qual;
			CheckQualifiers( nNewQualifiers, false );
			last_qual = nQualifiers;
			return( os::View::KeyUp( pzString, pzRawString, nQualifiers ) );
		}
		else if( (uint8)pzRawString[0] < 128 )
			keysym.sym = keymap[(uint8)pzRawString[0]];
		else
			keysym.sym = SDLK_UNKNOWN;

		keysym.mod = KMOD_NONE;
		keysym.unicode = 0;
		if (SDL_TranslateUNICODE)	
			keysym.unicode = Translate2Unicode( pzRawString );
		SDL_PrivateKeyboard(SDL_RELEASED, &keysym);
    	
//    	printf("Up %x %x %i\n", (uint)pzRawString[0], (uint)nQualifiers, keysym.sym );
    	os::View::KeyUp( pzString, pzRawString, nQualifiers );
	}
	void SetCursor( WMcursor *Cursor )
	{
		if( cursor && ( CurrentCursor != Cursor ) )
		{
			os::Application::GetInstance()->PopCursor();
			os::Application::GetInstance()->PushCursor( os::MPTR_MONO, Cursor->bits, 
				32, 32, os::IPoint( Cursor->hotx, Cursor->hoty ) );
		}
		CurrentCursor = Cursor;
	}

private:
	WMcursor *CurrentCursor;
	SDLKey keymap[128];
	uint32 last_qual;
	os::Bitmap *image;
	int xoff, yoff;
	bool cursor;
};

#endif /* _SDL_SylView_h */



