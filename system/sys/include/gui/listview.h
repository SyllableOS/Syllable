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

#ifndef __F_GUI_LISTVIEW_H__
#define __F_GUI_LISTVIEW_H__

#include <gui/control.h>
#include <util/variant.h>

#include <vector>
#include <string>
#include <functional>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

class ScrollBar;
class Button;
class Message;

class ListViewContainer;


/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ListViewRow
{
public:
    ListViewRow();
    virtual ~ListViewRow();

    virtual void AttachToView( View* pcView, int nColumn ) = 0;
    virtual void SetRect( const Rect& cRect, int nColumn ) = 0;
    
    virtual float GetWidth( View* pcView, int nColumn ) = 0;
    virtual float GetHeight( View* pcView ) = 0;
    virtual void  Paint( const Rect& cFrame, View* pcView, uint nColumn,
			 bool bSelected, bool bHighlighted, bool bHasFocus ) = 0;
    virtual bool  HitTest( View* pcView, const Rect& cFrame, int nColumn, Point cPos );
    virtual bool  IsLessThan( const ListViewRow* pcOther, uint nColumn ) const = 0;
    
    virtual void	SetCookie( Variant cCookie );
    virtual Variant	GetCookie( void );

    void		SetIsSelectable( bool bSelectable );
    bool		IsSelectable() const;
    bool		IsSelected() const;
    bool		IsHighlighted() const;
    void		SetIsVisible( bool bVisible );
    bool		IsVisible() const;
private:
    virtual void	__LVR_reserved_1__();
    virtual void	__LVR_reserved_2__();

private:
    ListViewRow& operator=( const ListViewRow &cRow )
	{
		m_cCookie = cRow.m_cCookie;
	};
    ListViewRow( const ListViewRow& );

    friend class ListView;
    friend class ListViewContainer;
    friend class ListViewCol;
    friend class std::vector<ListViewRow>;
    friend struct RowPosPred;

	void		SetHighlighted( bool bHigh );
	void		SetSelected( bool bSel );

    float	m_vYPos;
    float	m_vHeight;

	enum LVRFlags {
		F_SELECTABLE	= 0x01,
		F_SELECTED		= 0x02,
		F_HIGHLIGHTED	= 0x04,
		F_VISIBLE		= 0x08
	};

    uint16	m_nFlags;
    Variant m_cCookie;
    uint16	m_nReserved1;
    uint32	m_nReserved2[4];
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ListViewStringRow : public ListViewRow
{
public:
    ListViewStringRow() {}
    virtual ~ListViewStringRow() {}

    void		     AttachToView( View* pcView, int nColumn );
    void		     SetRect( const Rect& cRect, int nColumn );
    void		     AppendString( const String& cString );
    void	 	     SetString( int nIndex, const String& cString );
    const String& 	     GetString( int nIndex ) const;
    virtual float 	     GetWidth( View* pcView, int nColumn );
    virtual float  	     GetHeight( View* pcView );
    virtual void 	     Paint( const Rect& cFrame, View* pcView, uint nColumn,
				    bool bSelected, bool bHighlighted, bool bHasFocus );
    virtual bool	     IsLessThan( const ListViewRow* pcOther, uint nColumn ) const;
  
private:
    virtual void	__LVSR_reserved_1__();
    virtual void	__LVSR_reserved_2__();

private:
    ListViewStringRow& operator=( const ListViewStringRow& );
    ListViewStringRow( const ListViewStringRow& );

    std::vector< std::pair<String,float> > m_cStrings;
    uint32	m_nReserved[4];
};


/** Flexible multicolumn list view
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ListView : public Control
{
public:
    enum scroll_direction { SCROLL_UP, SCROLL_DOWN };
    enum { AUTOSCROLL_BORDER = 20 };
    enum { F_MULTI_SELECT = 0x0001,
	   F_NO_AUTO_SORT = 0x0002,
	   F_RENDER_BORDER = 0x0004,
	   F_DONT_SCROLL = 0x0008,
	   F_NO_HEADER = 0x0010,
	   F_NO_COL_REMAP = 0x0020,
	   F_NO_SORT = 0x0040 };
    enum { INV_HEIGHT = 0x01,
	   INV_WIDTH  = 0x02,
	   INV_VISUAL = 0x04 };

    typedef std::vector<ListViewRow*>::const_iterator const_iterator;
    typedef std::vector<int>			      column_map;
    
    ListView( const Rect& cFrame, const String& cTitle, uint32 nModeFlags = F_MULTI_SELECT | F_RENDER_BORDER,
	      uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
	      uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    
    ~ListView();

    virtual void LabelChanged( const String& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled );
    virtual bool Invoked( Message* pcMessage );
    
    virtual void		Invoked( int nFirstRow, int nLastRow );
    virtual void		SelectionChanged( int nFirstRow, int nLastRow );
    virtual bool		DragSelection( const Point& cPos );
   
    void			StartScroll( scroll_direction eDirection, bool bSelect );
    void			StopScroll();

    bool			IsMultiSelect() const;
    void			SetMultiSelect( bool bMulti );

    bool			IsAutoSort() const;
    void			SetAutoSort( bool bAuto );
    
    bool			HasBorder() const;
    void			SetRenderBorder( bool bRender );

    bool			HasColumnHeader() const;
    void			SetHasColumnHeader( bool bFlag );

    void			MakeVisible( int nRow, bool bCenter = true );
    int				InsertColumn( const char* pzTitle, int nWidth, int nPos = -1 );
    
    const column_map&		GetColumnMapping() const;
    void			SetColumnMapping( const column_map& cMap );
    
    void			InsertRow( int nPos, ListViewRow* pcRow, bool bUpdate = true );
    void			InsertRow( ListViewRow* pcRow, bool bUpdate = true );
    ListViewRow*		RemoveRow( int nIndex, bool bUpdate = true );
    void			InvalidateRow( int nRow, uint32 nFlags );
    uint			GetRowCount() const;
    ListViewRow*		GetRow( const Point& cPos ) const;
    ListViewRow*		GetRow( uint nIndex ) const;
    int				HitTest( const Point& cPos ) const;
    float			GetRowPos( int nRow );
    void			Clear();
    bool			IsSelected( uint nRow ) const;
    void			Select( int nFirst, int nLast, bool bReplace = true, bool bSelect = true );
    void			Select( int nRow, bool bReplace = true, bool bSelect = true );
    void			ClearSelection();
    
    void			Highlight( int nFirst, int nLast, bool bReplace, bool bHighlight = true );
    void			Highlight( int nRow, bool bReplace, bool bHighlight = true );

    void			SetCurrentRow( int nRow );
    void			Sort();
    int				GetFirstSelected() const;
    int				GetLastSelected() const;
    void	 		SetSelChangeMsg( Message* pcMsg );
    void	 		SetInvokeMsg( Message* pcMsg );
    Message*			GetSelChangeMsg() const;
    Message*			GetInvokeMsg() const;
    virtual void		Paint( const Rect& cUpdateRect );
    virtual void		FrameSized( const Point& cDelta );
    virtual void		KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void		AllAttached();
    virtual bool		HasFocus( void ) const;

	virtual void		SortRows( std::vector<ListViewRow*>* pcRows, int nColumn );
	virtual void		MouseDown(const Point&, uint32);
	virtual void		MouseUp( const Point & cPosition, uint32 nButton, Message * pcData );
	virtual void		MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData );

      // STL iterator interface to the rows.
    const_iterator begin() const;
    const_iterator end() const;
    
    void RefreshLayout();
   
private:
    virtual void	__LV_reserved_1__();
    virtual void	__LV_reserved_2__();
    virtual void	__LV_reserved_3__();
    virtual void	__LV_reserved_4__();
    virtual void	__LV_reserved_5__();
    
private:
    ListView& operator=( const ListView& );
    ListView( const ListView& );

    friend class ListViewContainer;
    friend class ListViewHeader;
    void	Layout();
    void  	AdjustScrollBars( bool bOkToHScroll = true );

	class Private;
	Private* m;
};

} // end of namespace


#endif // __F_GUI_LISTVIEW_H__

