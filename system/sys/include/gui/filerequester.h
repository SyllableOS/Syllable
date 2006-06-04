/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
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

#ifndef	__F_GUI_FILEREQUESTER_H__
#define	__F_GUI_FILEREQUESTER_H__

#include <sys/stat.h>
#include <gui/window.h>
#include <util/string.h>
namespace os
{
class DirectoryView;
class TextView;
class Button;
class StringView;
class DropdownMenu;
class Messenger;

class FileFilter
{
public:
  virtual bool FileOk( String pzPath, const struct stat* psStat ) = 0;
};

/** Generic file requester.
 * \ingroup gui
 * \par Description:
 * The filerequester is a seperate window that lets the user select files
 * that should be opened/saved. Please note that reading the selected
 * directory starts when the window is first shown. The following messages
 * are sent to the given message target:
 * M_FILE_REQUESTER_CANCELED - If the filerequester is closed without any action.
 * M_LOAD_REQUESTED - If a file has been selected to be loaded.
 * M_SAVE_REQUESTED - If a file has been selected to be saved.
 * \sa os::IconDirectoryView
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class FileRequester : public Window
{
public:
    enum file_req_mode_t { LOAD_REQ, SAVE_REQ };
    enum { NODE_FILE = 0x01, NODE_DIR = 0x02 };
  
    FileRequester( file_req_mode_t nMode = LOAD_REQ,
		   Messenger* pcTarget = NULL,
		   String cStartPath = "",
		   uint32 nNodeType = NODE_FILE,
		   bool bMultiSelect = true,
		   Message* pcMessage = NULL,
		   FileFilter* pcFilter = NULL,
		   bool  bModal = false,
		   bool bHideWhenDone = true,
		   String cOkLabel = "",
		   String cCancelLabel = "" );
	virtual ~FileRequester();
	
	void Show( bool bMakeVisible = true );

    virtual void	HandleMessage( Message* pcMessage );
    virtual void	FrameSized( const Point& cDelta );
	virtual bool	OkToQuit(void);

    void	SetPath( const String& cPath );
    String GetPath() const;
    

private:

    FileRequester& operator=( const FileRequester& );
    FileRequester( const FileRequester& );

	enum
	{ ID_PATH_CHANGED = 1, ID_SEL_CHANGED, ID_INVOKED, ID_CANCEL, ID_OK, ID_ALERT, ID_DROP_CHANGE, ID_UP_BUT = 0x55, ID_HOME_BUT = 0x56, ID_BACK_BUT = 0x57, ID_FORWARD_BUT = 0x58 };
	
	class Private;
	
	Private *m;
    
};

}

#endif // __F_GUI_FILEREQUESTER_H__
