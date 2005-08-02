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
#include <gui/view.h>
#include <util/message.h>
#include <util/string.h>
#include <storage/path.h>

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
	DOCK_UPDATE_FRAME = 100,
	DOCK_REMOVE,
};

class DockPlugin : public View
{
public:
	DockPlugin() : View( Rect(), "dock_plugin" ) {};
	~DockPlugin() {};
	
	virtual String GetIdentifier() = 0;
	virtual Point GetPreferredSize( bool bLargest ) = 0;
	virtual Path GetPath() = 0;
	
private:
};

typedef DockPlugin* init_dock_plugin( Path cPluginFile, Looper* pcDock );


}

#endif // __F_GUI_DOCKPLUGIN_H__












