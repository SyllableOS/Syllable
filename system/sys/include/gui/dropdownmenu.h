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
    DropdownMenu( const Rect& cFrame, const String& cName, uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
		  uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    ~DropdownMenu();

    void	SetEnable( bool bEnable = true );
    bool	IsEnabled() const;
    void	SetReadOnly( bool bFlag = true );
    bool	GetReadOnly() const;
  
    void	AppendItem( const String& cString );
    void	InsertItem( int nPosition, const String& cString );
    bool	DeleteItem( int nPosition );
    int		GetItemCount() const;
    void	Clear();
    const String& GetItem( int nItem ) const;

    int		GetSelection() const;
    void	SetSelection( int nItem, bool bNotify = true );

    const String&	GetCurrentString() const;
    void		SetCurrentString( const String& cString );

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
    virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void	AllAttached();
	virtual void Activated( bool bIsActive );
	virtual void SetTabOrder( int nOrder = NEXT_TAB_ORDER );
	virtual int GetTabOrder() const;
	virtual void WheelMoved( const Point& cDelta );
	virtual void FontChanged( Font* pcNewFont );

private:
	class DropdownView;
    friend class DropdownView;
    
	class DropdownTextView;
	friend class DropdownTextView;
  
    enum { ID_MENU_CLOSED = 1, ID_SELECTION_CHANGED, ID_STRING_CHANGED };
    void	OpenMenu();
  
    Window*   		     m_pcMenuWindow;
    DropdownTextView*    m_pcEditBox;
    Rect	    	     m_cArrowRect;
    std::vector<String> m_cStringList;
    Message*		     m_pcSelectionMsg;
    Message*		     m_pcEditMsg;
    int			     m_nSelection;
    bool	    	     m_bMenuOpen;
    bool		     m_bSendIntermediateMsg;
    bool		     m_bIsEnabled;
    
};

}

#endif // __F_GUI_DROPDOWNMENU_H__
