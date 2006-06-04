/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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

#ifndef __F_GUI_ICONDIRECTORYVIEW_H__
#define __F_GUI_ICONDIRECTORYVIEW_H__

#include <sys/stat.h>
#include <gui/control.h>
#include <gui/iconview.h>
#include <storage/path.h>
#include <util/string.h>

#include <vector>
#include <string>
#include <stack>

namespace os_priv
{
    class DirKeeper;
}

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

class Message;
class Bitmap;
class Image;
class Menu;
class RegistrarManager;
class NodeMonitor;

class DirectoryIconData : public IconData
{
public:
	DirectoryIconData()
	{
		m_pcMonitor = NULL;
	}
	~DirectoryIconData();

	os::String m_zPath;
	struct stat m_sStat;
	bool m_bBrokenLink;
	os::NodeMonitor* m_pcMonitor;
private:
	DirectoryIconData& operator=( const DirectoryIconData& );
	DirectoryIconData( const DirectoryIconData& );
};


/** Directory view suitable for file-requesters and other file browsers.
 * \ingroup gui
 * \par Description:
 *
 * \sa os::FileRequester
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/

class IconDirectoryView : public os::IconView
{
public:
    IconDirectoryView( const Rect& cFrame, const String& cPath, uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
    ~IconDirectoryView();
  
    void ReRead();
    void SetPath( const String& cPath );
    String GetPath() const;
    Image* GetDirIcon() const;
    bool	JobsPending() const;

    void	SetDirChangeMsg( Message* pcMsg );
	void	SetDirectoryLocked( bool bLocked );
	void	SetAutoLaunch( bool bAutoLaunch );

    virtual void  DirChanged( const String& cNewPath );
    virtual void  Invoked( uint nIcon, os::IconData *pcData );
    virtual void  DragSelection( os::Point cStartPoint );
    virtual void  OpenContextMenu( os::Point cPosition, bool bMouseOverIcon );
    virtual void  HandleMessage( Message* pcMessage );
    virtual void  AttachedToWindow();
    virtual void  DetachedFromWindow();
    virtual void  MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
	virtual void  MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void  KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
private:
    virtual void	__IDV_reserved1__();
    virtual void	__IDV_reserved2__();
    virtual void	__IDV_reserved3__();
    virtual void	__IDV_reserved4__();
    virtual void	__IDV_reserved5__();
private:
	enum { M_ADD_ENTRY, M_UPDATE_ENTRY, M_REMOVE_ENTRY, M_LAYOUT, M_REREAD, M_MOUNT = 100, M_UNMOUNT, 
    		M_NEW_DIR, M_DELETE, M_RENAME, M_OPEN_WITH, M_INFO, M_MOVE_TO_TRASH, M_EMPTY_TRASH, M_JOB_END, M_JOB_END_REREAD };

    IconDirectoryView& operator=( const IconDirectoryView& );
    IconDirectoryView( const IconDirectoryView& );

    friend class os_priv::DirKeeper;
    class Private;
    Private* m;
    
};

} // End of namespace

#endif // __F_GUI_DIRECTORYVIEW_H__



