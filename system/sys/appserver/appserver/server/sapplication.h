#ifndef __F_ATHEOS_GUI_H__
#define __F_ATHEOS_GUI_H__

/*
 *  The AtheOS application server
 *  Copyright (C) 1999  Kurt Skauen
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

#include <util/locker.h>

#include <set>
#include <string>
#include <vector>

using namespace os;

class SrvWindow;
class FontNode;
class BitmapNode;
class SrvSprite;
class SFontInstance;

namespace os {
    class Messenger;
}

class SrvApplication
{
public:
    SrvApplication( const char* pzName, proc_id hOwner, port_id hEventPort );
    ~SrvApplication();
    static SrvApplication*	FindApp( proc_id hProc );
    void		 Lock() 	{ m_cLocker.Lock(); }
    void		 Unlock()	{ m_cLocker.Unlock(); }
    bool		 IsLocked()	{ return( m_cLocker.IsLocked() ); }
    port_id	 GetReqPort() const { return( m_hReqPort ); }
    FontNode*	 GetFont( uint32 nID );
    void	PostUsrMessage( Message* pcMsg );
    static void ReplaceDecorators();
    static void NotifyWindowFontChanged( bool bToolWindow );
    static void NotifyColorCfgChanged();
    static void SetCursor( mouse_ptr_mode eMode, const IPoint& cHotSpot, const void* pImage, int nWidth, int nHeight );
    static void RestoreCursor();
    
private:
    void CreateBitmap( port_id hReply, int nWidth, int nHeight, color_space eColorSpc, uint32 nFlags );
    void DeleteBitmap( int hHandle );

    static SrvApplication* s_pcFirstApp;
    static Locker	   s_cAppListLock;
  
    static uint32	Entry( void* pData );
    void	DispatchMessage( Message* pcReq );
    bool	DispatchMessage( const void* pMsg, int nCode );
    void	Loop();

    void	AddWindow( SrvWindow* pcWnd );
    void	RemoveWindow( SrvWindow* pcWnd );

    std::string		   m_cName;
    SrvApplication*	   m_pcNext;
    thread_id		   m_hThread;
    proc_id		   m_hOwner;
    Locker		   m_cLocker;
    Locker		   m_cFontLock;
    port_id		   m_hReqPort;
    Messenger*		   m_pcAppTarget;

    struct Cursor
    {
	void*		m_pImage;
	IPoint		m_cSize;
	mouse_ptr_mode	m_eMode;
	IPoint		m_cHotSpot;
	SrvApplication* m_pcOwner;
    };

    static std::vector<Cursor*> s_cCursorStack;
    static bool			s_bBorderCursor;
    std::set<BitmapNode*> m_cBitmaps;
    std::set<SrvWindow*>  m_cWindows;
    std::set<FontNode*>   m_cFonts;
    std::set<SrvSprite*>  m_cSprites;
};

SrvApplication* get_active_app();

#endif // __F_ATHEOS_GUI_H__
