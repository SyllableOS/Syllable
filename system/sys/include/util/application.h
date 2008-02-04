/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef	__F_UTIL_APPLICATION_H__
#define	__F_UTIL_APPLICATION_H__

#include <util/looper.h>
#include <gui/window.h>
#include <gui/font.h>
#include <util/string.h>
#include <util/locale.h>
#include <util/catalog.h>
#include <gui/keyevent.h>

#include <vector>

namespace os {
    class Window;
    class Rect;
    class View;
}

/**  \brief os:: This namespace wrapps all AtheOS API classes
 *
 *	All function and classes defined by the AtheOS API is contained
 *	by the 'os' namespace.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *//////////////////////////////////////////////////////////////////////////////

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

/** Singleton class representing an application
 * \ingroup util
 * \par Description:
 *	Before you can use any of the classes in the AtheOS API you must instantiate
 *	an application object. The application object establish the connection with
 *	the appserver, and are responcible for all application level comunication
 *	after the connection is established.
 *
 *	os::Application inherit from Looper, but it does behave different from
 *	other loopers in the way that it does not spawn a new thread when the
 *	Run() member is called. Insetead it steel the thread calling the run member
 *	(normally the main thread) and let it run the message loop.
 *
 * \sa os::Window, os::View, os::Looper
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Application : public Looper
{
public:
    Application( const char* pzMimeType );
    virtual		~Application();
    static Application*	GetInstance();
    uint32		GetQualifiers();

    virtual void	HandleMessage( Message* pcMessage );

    virtual void	__reserved1__();
    virtual void	__reserved2__();
    virtual void	__reserved3__();
    virtual void	__reserved4__();
    virtual void	__reserved5__();
    virtual void	__reserved6__();
    virtual void	__reserved7__();
    virtual void	__reserved8__();
    virtual void	__reserved9__();
    virtual void	__reserved10__();
    
    void PushCursor( mouse_ptr_mode eMode, void* pImage, int nWidth, int nHeight, const IPoint& cHotSpot = IPoint(0,0)  );
    void PopCursor();
    
    bigtime_t	GetIdleTime();
    void		GetKeyboardConfig( String* pcKeymapName, int* pnKeyDelay, int* pnKeyRepeat );
    status_t	SetKeymap( const char* pzName );
    status_t	SetKeyboardTimings( int nDelay, int nRepeat );
    
    int			GetScreenModeCount();
    int			GetScreenModeInfo( int nIndex, screen_mode* psMode );

    void		SetWindowDecorator( const char* pzPath );
    void		CommitColorConfig();
  
    thread_id	Run();

	port_id		GetServerPort()	const;
	port_id		GetAppPort() const;

	const Catalog* GetCatalog() const;
	void SetCatalog( Catalog* pcCatalog );
	bool SetCatalog( const String& cCatalogName );

	Locale* GetApplicationLocale() const;
	void SetApplicationLocale( const Locale& pcLocale );


	void RegisterKeyEvent(const os::KeyboardEvent&);
	void UnregisterKeyEvent(const os::String& cEvent);
	int GetCurrentKeyShortcuts(std::vector<os::KeyboardEvent> *pcTable);
private:
    friend class Window;
    friend class Desktop;
    friend class Bitmap;
    friend class Sprite;
    friend class View;
    friend class Font;
    friend class AppserverConfig;
  
      // Support for the Window class:
    port_id CreateWindow( View* pcTopView, const Rect& cFrame, const String& cTitle,
			  uint32 nFlags, uint32 nDesktopMask, port_id hEventPort, int* phTopView );
    port_id CreateWindow( View* pcTopView, int hBitmapHandle, int* phTopView );

    void  AddWindow( Window* pcWindow );
    void  RemoveWindow( Window* pcWindow );

      // Support for the Font class:
    int		GetFontConfigNames( std::vector<String>* pcTable ) const;
    int		GetDefaultFont( const String& cName, font_properties* psProps ) const;
    int		SetDefaultFont( const String& cName, const font_properties& sProps );
    int		AddDefaultFont( const String& cName, const font_properties& sProps );
    
    int		GetFontFamilyCount();
    status_t	GetFontFamily( int nIndex, char* pzFamily );
    int		GetFontStyleCount( const char* pzFamily );
    status_t	GetFontStyle( const char* pzFamily, int nIndex, char* pzStyle, uint32* pnFlags = NULL );
    
    int   LockDesktop( int nDesktop, Message* pcDesktopParams );
    int	  UnlockDesktop( int nCookie );
    int   SwitchDesktop( int nDesktop );
    void  SetScreenMode( int nDesktop, uint32 nValidityMask, screen_mode* psMode );
  
    int  CreateBitmap( int nWidth, int nHeight, color_space eColorSpc,
		      uint32 nFlags, int* phHandle, int* phArea );
	int  CloneBitmap( int hHandle, int* phHandle, area_id* phArea, uint32 *pnFlags, int* pnWidth,
				int* pnHeight, color_space* peColorSpc );
    int  DeleteBitmap( int nBitmap );
    int  CreateSprite( const Rect& cFrame, int nBitmapHandle, uint32* pnHandle );
    void DeleteSprite( uint32 nSprite );
    void MoveSprite( uint32 nSprite, const Point& cNewPos );

private:
    static Application*	 s_pcInstance;

    class Private;
    Private* m;
};

}

#endif	// __F_UTIL_APPLICATION_H__






