/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2000  Kurt Skauen
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

#ifndef __F_GUI_DROPDOWNMENU_H__
#define __F_GUI_DROPDOWNMENU_H__

#include <gui/view.h>
#include <gui/font.h>
#include <util/invoker.h>
#include <gui/textview.h>
#include <gui/scrollbar.h>

#include <vector>
#include <string>

namespace os
{
#if 0
} // Fool the Emacs auto indenter
#endif

//class TextView;
class DropdownMenu;

/** Edit box with an asociated item-menu.
 * \ingroup gui
 * \par Description:
 *
 * \sa TextView, Invoker
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class DropdownMenu : public View, public Invoker
{
public:
    DropdownMenu( const Rect& cFrame, const char* pzName, uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
		  uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    ~DropdownMenu();

    void	SetEnable( bool bEnable = true );
    bool	IsEnabled() const;
    void	SetReadOnly( bool bFlag = true );
    bool	GetReadOnly() const;
  
    void	AppendItem( const char* pzString );
    void	InsertItem( int nPosition, const char* pzString );
    bool	DeleteItem( int nPosition );
    int		GetItemCount() const;
    void	Clear();
    const std::string& GetItem( int nItem ) const;

    int		GetSelection() const;
    void	SetSelection( int nItem, bool bNotify = true );

    const std::string&	GetCurrentString() const;
    void		SetCurrentString( const std::string& cString );

    void 	SetMinPreferredSize( int nWidthChars );
    int		GetMinPreferredSize() const;
    void 	SetMaxPreferredSize( int nWidthChars );
    int		GetMaxPreferredSize() const;
  
    void	SetSelectionMessage( Message* pcMsg );
    Message*	GetSelectionMessage() const;
    void	SetSendIntermediateMsg( bool bFlag );
    bool	GetSendIntermediateMsg() const;
    void	SetEditMessage( Message* pcMsg );
    Message*	GetEditMessage() const;
  
    virtual void	HandleMessage( Message* pcMessage );
  
    virtual void	Paint( const Rect& cUpdateRect );
    virtual Point	GetPreferredSize( bool bLargest ) const;
    virtual void	FrameSized( const Point& cDelta );
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
//    virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void	AllAttached();
  
private:
    class DropdownView : public View
    {
    	public:
			DropdownView( DropdownMenu* pcParent );
		
			virtual void	Paint( const Rect& cUpdateRect );
			virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
			virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );
			virtual void	MouseMove( const Point& cPosition, int nCode, uint32 nButtons, Message* pcData );
			virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
			virtual void	AllAttached();
			virtual void	Activated( bool bIsActive );
			
		    virtual void	HandleMessage( Message* pcMessage );
  
	    private:
			friend class DropdownMenu;
			DropdownMenu* m_pcParent;
			Point	      m_cContentSize;
			font_height   m_sFontHeight;
			float	      m_vGlyphHeight;
			int	      m_nOldSelection;
			int	      m_nCurSelection;
		    int		m_nScrollPos;
		    ScrollBar *m_pcScrollBar;
		    Point m_cOldPosition;
    };
    friend class DropdownView;
    
    class DropdownTextView : public os::TextView
    {
    	public:
    		DropdownTextView(DropdownMenu * pcParent, const Rect &cFrame, const char *pzTitle, const char *pzBuffer, uint32 nResizeMask=CF_FOLLOW_LEFT|CF_FOLLOW_TOP, uint32 nFlags=WID_WILL_DRAW|WID_FULL_UPDATE_ON_RESIZE);
			~DropdownTextView() { };
    		void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    	private:
    		DropdownMenu *m_pcParent;
    };
	friend class DropdownTextView;
  
    enum { ID_MENU_CLOSED = 1, ID_SELECTION_CHANGED, ID_STRING_CHANGED };
    void	OpenMenu();
  
    Window*   		     m_pcMenuWindow;
    DropdownTextView*    m_pcEditBox;
    Rect	    	     m_cArrowRect;
    std::vector<std::string> m_cStringList;
    Message*		     m_pcSelectionMsg;
    Message*		     m_pcEditMsg;
    int			     m_nSelection;
    bool	    	     m_bMenuOpen;
    bool		     m_bSendIntermediateMsg;
    bool		     m_bIsEnabled;
    
};

}

#endif // __F_GUI_DROPDOWNMENU_H__
