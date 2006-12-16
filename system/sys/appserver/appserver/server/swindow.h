#ifndef	INTERFACE_SLAYER_HPP
#define	INTERFACE_SLAYER_HPP

/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "layer.h"
#include <util/array.h>
#include <util/locker.h>
#include <util/message.h>

#include <string>

namespace os {
    class Messenger;
    class Locker;
    class WindowDecorator;
    struct WR_Render_s;
};

class SrvApplication;
class WndBorder;
class SrvBitmap;
class SrvSprite;
class SrvEvent_s;



class SrvWindow
{
public:
    SrvWindow( const char* pzTitle, void* pTopView, uint32 nFlags, uint32 nDesktopMask,
	       const os::Rect& cRect, SrvApplication* pcApp, port_id hEventPort );
    SrvWindow( SrvApplication* pcApp, SrvBitmap* pcBitmap );
    ~SrvWindow();

	os::String	BuildEventIDString();
	void	PostEvent( bool bIconChanged = false );
    void	PostUsrMessage( os::Message* pcMsg );
	os::WindowDecorator*	GetDecorator() const;
	WndBorder*	GetWndBorder() const;
    void	ReplaceDecorator();
    void	NotifyWindowFontChanged( bool bToolWindow );
    void	Quit();
    const char*	GetTitle() const { return( m_cTitle.c_str() ); }
    SrvApplication* GetApp() const { return( m_pcApp ); }
    os::Messenger*  GetAppTarget( void ) const	{ return( m_pcAppTarget ); 	}
    void	Show( bool bShow );
    void	MakeFocus( bool bFocus );
    bool	HasFocus() const;
    void	SetBitmap( SrvBitmap* pcBitmap ) { m_pcTopView->SetBitmap( pcBitmap ); }

    void	DesktopActivated( int nNewDesktop, bool bActivated, const os::IPoint cNewRes, os::color_space eColorSpace );
    void	WindowActivated( bool bActive );
    void	ScreenModeChanged( const os::IPoint cNewRes, os::color_space eColorSpace );
  
    void	SetFrame( const os::Rect& cFrame );
    os::Rect	GetFrame( bool bClient = true ) const;
  
    Layer*	GetTopView() const;
    Layer*	GetClientView() const { return( m_pcTopView ); }
    
    uint32	GetDesktopMask() const;
    void	SetDesktopMask( uint32 nMask );
  
    thread_id	Run();
    status_t	Lock();
    status_t	Unlock();
    bool	IsLocked( void );
    port_id	GetMsgPort( void )	{ return( m_hMsgPort ); }

    uint32	GetFlags() const 	{ return( m_nFlags ); }
    void	SetFlags( uint32 nFlags ) { m_nFlags = nFlags; }
    bool	DispatchMessage( os::Message* pcReq );
    bool	DispatchMessage( const void* psMsg, int nCode );
    bool	IsOffScreen() const	{ return( m_bOffscreen ); }
    int		GetPaintCounter() const { return( atomic_read( &m_nPendingPaintCounter ) ); }
	void	IncPaintCounter() { atomic_inc( &m_nPendingPaintCounter ); }
	
    bool	HasPendingSizeEvents( Layer* pcLayer );
    
    static void SendKbdEvent( int nKeyCode, uint32 nQual, const char* pzConvString, const char* zRawString );
    static void HandleInputEvent( os::Message* pcEvent );
    static void HandleMouseTransaction();
    
	SrvBitmap* GetIcon() { return( m_pcIcon ); }
    
    bool	IsMinimized() { return( m_bMinimized ); }
    void	SetMinimized( bool bMinimized ) { m_bMinimized = bMinimized; }
    
    struct DeskTopState_s
    {
	DeskTopState_s() { m_pcNextWindow = NULL; }
	os::Rect   m_cFrame;
	SrvWindow* m_pcNextWindow;
	SrvWindow* m_pcNextSystemWindow;
    } m_asDTState[32];
    
    
  
private:
    void		R_Render( os::WR_Render_s* psPkt );
    void		MouseMoved( os::Message* pcEvent, int nTransit );
    void		MouseDown( os::Message* pcEvent );
    void		MouseUp( os::Message* pcEvent, bool bSendDragMsg );

    static void		HandleMouseDown( const os::Point& cMousePos, int nButton, os::Message* pcEvent );
    static void		HandleMouseMoved( const os::Point& cMousePos, os::Message* pcEvent );
    static void		HandleMouseUp( const os::Point& cMousePos, int nButton, os::Message* pcEvent );
    
 
    void		Loop( void );
    static uint32	Entry( void* pData );

    static SrvWindow*	s_pcLastMouseWindow;
    static SrvWindow*	s_pcDragWindow;
    static SrvSprite*	s_pcDragSprite;
    static os::Message	s_cDragMessage;
    static bool		s_bIsDragging;
  
    std::string		m_cTitle;
    uint32		m_nFlags;
    uint32		m_nDesktopMask;
    SrvBitmap*		m_pcUserBitmap;
    SrvApplication*	m_pcApp;
    Layer*			m_pcTopView;
    WndBorder*		m_pcWndBorder;
    os::WindowDecorator* m_pcDecorator;
	SrvEvent_s*		m_pcEvent;
    bigtime_t		m_nLastHitTime;	// Time of last mouse click
    bool			m_bOffscreen;	// True for bitmap windows
	atomic_t		m_nPendingPaintCounter;
    thread_id		m_hThread;
    port_id			m_hMsgPort;	// Requests from application
    port_id			m_hEventPort;	// Events to application
    os::Locker		m_cMutex;
    os::Messenger*	m_pcAppTarget;
    SrvBitmap*		m_pcIcon;
    bool			m_bMinimized;
};

#endif	//	INTERFACE_SLAYER_HPP


