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

#ifndef __F_GUI_DIRECTORYVIEW_H__
#define __F_GUI_DIRECTORYVIEW_H__

#include <sys/stat.h>
#include <gui/listview.h>
#include <storage/path.h>

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


/** Directory browser control.
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class FileRow : public ListViewRow
{
public:
    FileRow( Bitmap* pcBitmap, const String& cName, const struct stat& sStat ) : m_cName(cName.c_str()) {
	m_sStat = sStat; m_pcIconBitmap = pcBitmap;
    }
    virtual void AttachToView( View* pcView, int nColumn );
    virtual void SetRect( const Rect& cRect, int nColumn );
    
    virtual float GetWidth( View* pcView, int nIndex ) const;
    virtual float GetHeight( View* pcView ) const;
    virtual void  Paint( const Rect& cFrame, View* pcView, uint nColumn,
			 bool bSelected, bool bHighlighted, bool bHasFocus );
    virtual bool  HitTest( View* pcView, const Rect& cFrame, int nColumn, Point cPos );
    virtual bool  IsLessThan( const ListViewRow* pcOther, uint nColumn ) const;
    const String GetName() const { return( m_cName ); }
    struct stat GetFileStat() const { return( m_sStat ); }
private:
    FileRow& operator=( const FileRow& );
    FileRow( const FileRow& );

    friend class DirectoryView;
    std::string		m_cName;
    struct stat	m_sStat;
    uint8       	m_anIcon[16*16*4];
    Bitmap*		m_pcIconBitmap;
    float		m_avWidths[7];
};

/** Directory view suitable for file-requesters and other file browsers.
 * \ingroup gui
 * \par Description:
 *
 * \sa os::FileRequester
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class DirectoryView : public ListView
{
public:
    DirectoryView( const Rect& cFrame, const String& cPath,
		   uint32 nModeFlags = F_MULTI_SELECT | F_RENDER_BORDER,
		   uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
		   uint32 nViewFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    ~DirectoryView();
  
    void ReRead();
    void SetPath( const String& cPath );
    String GetPath() const;

    FileRow*	GetFile( int nRow ) const { return( static_cast<FileRow*>(GetRow(nRow)) ); }
    void	SetDirChangeMsg( Message* pcMsg );
//  void	SetDirChangeTarget( const Handler* pcHandler, const Looper* pcLooper = NULL );
//  void	SetDirChangeTarget( const Messenger& cTarget );

    virtual void  DirChanged( const String& cNewPath );
    virtual void  Invoked( int nFirstRow, int nLastRow );
    virtual bool  DragSelection( const Point& cPos );
    virtual void  HandleMessage( Message* pcMessage );
    virtual void  AttachedToWindow();
    virtual void  DetachedFromWindow();
    virtual void  MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
    virtual void  MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData );
    virtual void  KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
private:
    DirectoryView& operator=( const DirectoryView& );
    DirectoryView( const DirectoryView& );

    enum { M_CLEAR, M_ADD_ENTRY, M_UPDATE_ENTRY, M_REMOVE_ENTRY };
    friend class os_priv::DirKeeper;
    
    static int32 ReadDirectory( void* pData );
    void	 PopState();
  
    struct State {
	State( ListView* pcView, const char* pzPath );
	std::string	      	 m_cPath;
	uint 	      	         m_nScrollOffset;
	std::vector<std::string> m_cSelection;
    };
    struct ReadDirParam {
	ReadDirParam( DirectoryView* pcView ) { m_pcView = pcView; m_bCancel = false; }
	DirectoryView* m_pcView;
	bool	   m_bCancel;
    };
    ReadDirParam*       m_pcCurReadDirSession;
    Message*	        m_pcDirChangeMsg;
    Path	        m_cPath;
//    Directory	      m_cDirectory;
    os_priv::DirKeeper* m_pcDirKeeper;
    std::string	        m_cSearchString;
    bigtime_t	        m_nLastKeyDownTime;
    std::stack<State>   m_cStack;
    Bitmap*	        m_pcIconBitmap;
};

} // End of namespace

#endif // __F_GUI_DIRECTORYVIEW_H__
