/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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
namespace os
{
class DirectoryView;
class TextView;
class Button;
class StringView;
class DropdownMenu;

class FileFilter
{
public:
  virtual bool FileOk( const char* pzPath, const struct ::stat* psStat ) = 0;
};

/** Generic file requester.
 * \ingroup gui
 * \par Description:
 *
 * \sa os::DirectoryView
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class FileRequester : public Window
{
public:
    enum file_req_mode_t { LOAD_REQ, SAVE_REQ };
    enum { NODE_FILE = 0x01, NODE_DIR = 0x02 };
  
    FileRequester( file_req_mode_t nMode = LOAD_REQ,
		   Messenger* pcTarget = NULL,
		   const char* pzPath = NULL,
		   uint32 nNodeType = NODE_FILE,
		   bool bMultiSelect = true,
		   Message* pcMessage = NULL,
		   FileFilter* pcFilter = NULL,
		   bool  bModal = false,
		   bool bHideWhenDone = true,
		   const char* pzOkLabel = NULL,
		   const char* pzCancelLabel = NULL );
    virtual void	HandleMessage( Message* pcMessage );
    virtual void	FrameSized( const Point& cDelta );

    void	SetPath( const std::string& cPath );
    std::string GetPath() const;
private:
    void Layout();
    enum { ID_PATH_CHANGED = 1, ID_SEL_CHANGED, ID_INVOKED, ID_CANCEL, ID_OK, ID_ALERT, ID_DROP_CHANGE };

    Message*	 m_pcMessage;
    Messenger*	 m_pcTarget;
  
    file_req_mode_t m_nMode;
    uint32	    m_nNodeType;
    bool	    m_bHideWhenDone;
    DirectoryView*  m_pcDirView;
    TextView*	    m_pcPathView;
    Button*	    m_pcOkButton;
    Button*	    m_pcCancelButton;
    StringView* m_pcFileString;
    StringView* m_pcTypeString;
    DropdownMenu* m_pcTypeDrop;
    
};

}

#endif // __F_GUI_FILEREQUESTER_H__



