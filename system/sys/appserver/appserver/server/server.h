#ifndef	INTERFACE_SERVER_HPP
#define	INTERFACE_SERVER_HPP

/*
 *  The AtheOS application server
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#include <atheos/types.h>
#include <atheos/time.h>
#include <atheos/mouse.h>

#include <gui/region.h>
#include <gui/gfxtypes.h>
#include <gui/bitmap.h>
#include <util/array.h>
#include <util/messagequeue.h>

#include "ddriver.h"
#include "windowdecorator.h"
#include "toplayer.h"
#include "keyevent.h"

typedef	struct TextFont	TextFont_s;

	
class SrvWindow;
class Layer;
class AppServer;
class SFontInstance;
class DisplayDriver;
class SrvBitmap;
class SrvEvents;

namespace os
{
	class Locker;
	class Message;
	class ShortcutKey;
};

extern ::TopLayer* g_pcTopView;

extern DisplayDriver*	g_pcDispDrv;

extern Array<Layer>*	g_pcLayers;
extern os::Locker*		g_pcWinListLock;
extern SrvEvents*		g_pcEvents;

void config_changed();

class AppServer
{
public:
    AppServer();
    ~AppServer();

    static AppServer*		GetInstance();
    os::WindowDecorator*	CreateWindowDecorator( Layer* pcView, uint32 nFlags );
  
    void					Run( void );
    static int32 			CloseWindows( void* pData );
    DisplayDriver*			GetDisplayDriver( int nIndex )	{ return( m_pcDispDriver ); }

    void					SendKeyCode( int nKeyCode, int nQualifiers );

    FontNode*				GetWindowTitleFont() const { return( m_pcWindowTitleFont ); }
    FontNode*				GetToolWindowTitleFont() const { return( m_pcToolWindowTitleFont ); }
    
   
    void					R_ClientDied( thread_id hThread );

    void					ResetEventTime();
    bigtime_t				GetIdleTime() const { return( get_system_time() - m_nLastEvenTime ); }
    
    
    int						LoadWindowDecorator( const std::string& cPath );

/*why is this public*/
public:
    port_id 				m_hWndReplyPort;

private:
	void					RescanFonts(void);
	void					SwitchDesktop(int nDesktop, bool bBringWindow = true );
	
    int						LoadDecorator( const std::string& cPath, os::op_create_decorator** ppfCreate, os::op_unload_decorator** ppfUnload );
	void					DispatchMessage( os::Message* pcReq );
    
private:
    static AppServer*			s_pcInstance;

    DisplayDriver*				m_pcDispDriver;
	os::op_create_decorator*	m_pfDecoratorCreator;
	os::op_unload_decorator*	m_pfDecoratorUnload;

	FontNode*					m_pcWindowTitleFont;
	FontNode*					m_pcToolWindowTitleFont;
    
	int							m_hCurrentDecorator;
	int 						nWatchNode;
	bigtime_t					m_nLastEvenTime;
	port_id		 				m_hRequestPort;
    
    
    std::vector<KeyEvent_s> cKeyEvents;
    
    /*these are all the events that the appserver can handle*/
    SrvEvent_s* 				m_pcProcessQuitEvent;
    SrvEvent_s* 				m_pcClipboardEvent;
    SrvEvent_s*					m_pcScreenshotEvent;
};

#endif	/*	INTERFACE_SERVER_HPP	*/









