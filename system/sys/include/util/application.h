/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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

#include <vector>
#include <string>

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
    void	GetKeyboardConfig( std::string* pcKeymapName, int* pnKeyDelay, int* pnKeyRepeat );
    status_t	SetKeymap( const char* pzName );
    status_t	SetKeyboardTimings( int nDelay, int nRepeat );
    
    int		GetScreenModeCount();
    int		GetScreenModeInfo( int nIndex, screen_mode* psMode );

    void	SetWindowDecorator( const char* pzPath );
    void	CommitColorConfig();
  
    thread_id	Run();

   port_id		GetServerPort()	const;
   port_id		GetAppPort() const;

private:
    friend class Window;
    friend class Desktop;
    friend class Bitmap;
    friend class Sprite;
    friend class View;
    friend class Font;
    friend class AppserverConfig;
  
      // Support for the Window class:
    port_id CreateWindow( View* pcTopView, const Rect& cFrame, const std::string& cTitle,
			  uint32 nFlags, uint32 nDesktopMask, port_id hEventPort, int* phTopView );
    port_id CreateWindow( View* pcTopView, int hBitmapHandle, int* phTopView );

    void  AddWindow( Window* pcWindow );
    void  RemoveWindow( Window* pcWindow );

      // Support for the Font class:
    int		GetFontConfigNames( std::vector<string>* pcTable ) const;
    int		GetDefaultFont( const std::string& cName, font_properties* psProps ) const;
    int		SetDefaultFont( const std::string& cName, const font_properties& sProps );
    int		AddDefaultFont( const std::string& cName, const font_properties& sProps );
    
    int		GetFontFamilyCount();
    status_t	GetFontFamily( int nIndex, char* pzFamily );
    int		GetFontStyleCount( const char* pzFamily );
    status_t	GetFontStyle( const char* pzFamily, int nIndex, char* pzStyle, uint32* pnFlags = NULL );
    
    int   LockDesktop( int nDesktop, Message* pcDesktopParams );
    int	  UnlockDesktop( int nCookie );
    int   SwitchDesktop( int nDesktop );
    void  SetScreenMode( int nDesktop, uint32 nValidityMask, screen_mode* psMode );
  
    int CreateBitmap( int nWidth, int nHeight, color_space eColorSpc,
		      uint32 nFlags, int* phHandle, int* phArea );
    int  DeleteBitmap( int nBitmap );
    int  CreateSprite( const Rect& cFrame, int nBitmapHandle, uint32* pnHandle );
    void DeleteSprite( uint32 nSprite );
    void MoveSprite( uint32 nSprite, const Point& cNewPos );

private:
    static Application*	 s_pcInstance;

    class Private;
    Private* m;
//    uint32   __reserver__[5];
/*    port_id		 m_hServerPort;
    port_id		 m_hSrvAppPort;
    port_id		 m_hReplyPort;
    std::vector<Window*> m_cWindows;*/
};

}

#endif	// __F_UTIL_APPLICATION_H__



