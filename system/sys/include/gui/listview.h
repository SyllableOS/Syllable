/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
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
    
    void		SetIsSelectable( bool bSelectable );
    bool		IsSelectable() const;
    bool		IsSelected() const;
    bool		IsHighlighted() const;
    void		SetIsVisible( bool bVisible );
    bool		IsVisible() const;
private:
    friend class ListView;
    friend class ListViewContainer;
    friend class ListViewCol;
    friend class std::vector<ListViewRow>;
    friend struct RowPosPred;
    float	m_vYPos;
    float	m_vHeight;

	void		SetHighlighted( bool bHigh );
	void		SetSelected( bool bSel );

	enum LVRFlags {
		F_SELECTABLE	= 0x01,
		F_SELECTED		= 0x02,
		F_HIGHLIGHTED	= 0x04,
		F_VISIBLE		= 0x08
	};

    uint16	m_nFlags;
    uint8	m_nReserved;
/*    bool	m_bIsSelectable;
    bool	m_bSelected;
    bool	m_bHighlighted;*/
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
    void		     AppendString( const std::string& cString );
    void	 	     SetString( int nIndex, const std::string& cString );
    const std::string& 	     GetString( int nIndex ) const;
    virtual float 	     GetWidth( View* pcView, int nColumn );
    virtual float  	     GetHeight( View* pcView );
    virtual void 	     Paint( const Rect& cFrame, View* pcView, uint nColumn,
				    bool bSelected, bool bHighlighted, bool bHasFocus );
    virtual bool	     IsLessThan( const ListViewRow* pcOther, uint nColumn ) const;
  
private:
    std::vector< std::pair<std::string,float> > m_cStrings;
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
	   F_NO_COL_REMAP = 0x0020 };
    enum { INV_HEIGHT = 0x01,
	   INV_WIDTH  = 0x02,
	   INV_VISUAL = 0x04 };

    typedef std::vector<ListViewRow*>::const_iterator const_iterator;
    typedef std::vector<int>			      column_map;
    
    ListView( const Rect& cFrame, const char* pzTitle, uint32 nModeFlags = F_MULTI_SELECT | F_RENDER_BORDER,
	      uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
	      uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    
    ~ListView();

    virtual void LabelChanged( const std::string& cNewLabel );
    virtual void EnableStatusChanged( bool bIsEnabled );
    virtual bool Invoked( Message* pcMessage );
    
    virtual void		Invoked( int nFirstRow, int nLastRow );
    virtual void		SelectionChanged( int nFirstRow, int nLastRow );
    virtual bool		DragSelection( const Point& cPos );

    virtual void __reserved1__();
    virtual void __reserved2__();
    virtual void __reserved3__();
    virtual void __reserved4__();
    
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

      // STL iterator interface to the rows.
    const_iterator begin() const;
    const_iterator end() const;
    
    void RefreshLayout();
   
private:
    friend class ListViewContainer;
    friend class ListViewHeader;
    void	Layout();
    void  	AdjustScrollBars( bool bOkToHScroll = true );
  
    ListViewContainer* m_pcMainView;
    ListViewHeader*    m_pcHeaderView;
    ScrollBar*	       m_pcVScroll;
    ScrollBar*	       m_pcHScroll;
    Message*	       m_pcSelChangeMsg;
    Message*	       m_pcInvokeMsg;
//    uint32	       m_nModeFlags;
};

} // end of namespace


#endif // __F_GUI_LISTVIEW_H__
