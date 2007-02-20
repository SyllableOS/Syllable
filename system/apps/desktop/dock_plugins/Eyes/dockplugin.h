/*  libsyllable.so - the highlevel API library for Syllable
 *  Dock plugin interface
 *  Copyright (C) 2003 Arno Klenke
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
 
 

#ifndef __F_GUI_DOCKPLUGIN_H__
#define __F_GUI_DOCKPLUGIN_H__

#include <atheos/types.h>
#include <atheos/filesystem.h>
#include <gui/view.h>
#include <util/message.h>
#include <util/string.h>
#include <util/looper.h>
#include <storage/path.h>
#include <unistd.h>

namespace os {
#if 0
} // Fool Emacs auto-indent
#endif


enum {
	/* Public message interface */
	DOCK_GET_PLUGINS,
	DOCK_SET_PLUGINS,
	DOCK_GET_POSITION,
	DOCK_SET_POSITION,
	
	/* Plugin messages */
	DOCK_ADD_VIEW = 100,
	DOCK_REMOVE_VIEW, 
	DOCK_UPDATE_FRAME,
	DOCK_REMOVE_PLUGIN
};

class DockPlugin
{
public:
	DockPlugin() 
	{
		m_nViewCount = 0;
		m_bDeleted = false;
	};
	virtual ~DockPlugin() {};
	virtual status_t		Initialize() = 0;
	virtual void			Delete() = 0;
	virtual String			GetIdentifier() = 0;
	
	/* Hooks to notify plugins of pertinent events */
	virtual void ScreenModeChanged( const IPoint& cNewRes, color_space eColorSpace ) {};
	virtual void DesktopActivated( int nDesktop, bool bActive ) {};
		
	image_id GetPluginId()
	{
		return( m_nPlugin );
	}
	os::Path GetPath()
	{
		char zPath[PATH_MAX];
	
		/* Get path of the application */
		int nFd = open_image_file( GetPluginId() );
		if( nFd < 0 )
			return( os::Path() );
	
		get_directory_path( nFd, zPath, PATH_MAX );
		close( nFd );
		return( os::Path( zPath ) );
	}
	void SetApp( os::Looper* pcApp, image_id nPlugin )
	{
		m_pcApp = pcApp;
		m_nPlugin = nPlugin;
	}
	os::Looper* GetApp()
	{
		return( m_pcApp );
	}
	void AddView( os::View* pcView )
	{
		os::Message cMsg( DOCK_ADD_VIEW );
		cMsg.AddPointer( "plugin", this );
		cMsg.AddPointer( "view", pcView );
		m_pcApp->PostMessage( &cMsg, m_pcApp );
	}
	void RemoveView( os::View* pcView, thread_id nThreadId = -1 )
	{
		os::Message cMsg( DOCK_REMOVE_VIEW );
		cMsg.AddPointer( "plugin", this );
		cMsg.AddPointer( "view", pcView );
		cMsg.AddInt32( "thread", nThreadId );
		m_pcApp->PostMessage( &cMsg, m_pcApp );
		
	}
	int SetViewCount( int nCount )
	{
		m_nViewCount = nCount;
	}
	int GetViewCount()
	{
		return( m_nViewCount );
	}
	void SetDeleted()
	{
		m_bDeleted = true;
	}
	bool GetDeleted()
	{
		return( m_bDeleted );
	}
private:
	os::Looper*	m_pcApp;
	image_id	m_nPlugin;
	int			m_nViewCount;
	bool		m_bDeleted;
};

typedef DockPlugin* init_dock_plugin();


}

#endif // __F_GUI_DOCKPLUGIN_H__












