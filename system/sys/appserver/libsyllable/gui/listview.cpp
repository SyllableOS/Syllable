/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#include <stdio.h>

#include <gui/listview.h>
#include <atheos/time.h>

#include <gui/scrollbar.h>
#include <gui/button.h>
#include <gui/font.h>
#include <util/message.h>
#include <util/application.h>

#include <limits.h>
#include <algorithm>

using namespace os;

struct RowContentPred
{				// : public binary_function<ListViewRow,ListViewRow, bool> {
	RowContentPred( int nColumn )
	{
		m_nColumn = nColumn;
	}
	bool operator() ( const ListViewRow * x, const ListViewRow * y )const
	{
		return ( x->IsLessThan( y, m_nColumn ) );
	}
	int m_nColumn;
};

namespace os
{
	struct RowPosPred
	{			// : public binary_function<ListViewRow,ListViewRow, bool> {
		bool operator() ( const ListViewRow * x, const ListViewRow * y )const
		{
//			cout << "x: " << x->m_vYPos << " y: " << y->m_vYPos << " r: " << ( ( x->m_vYPos < y->m_vYPos ) && x->IsVisible() ) << endl;
			return ( ( x->m_vYPos < y->m_vYPos ) && x->IsVisible() );
		}
	};
}

/*
  ListView -+
            |
            +-- ListViewHeader -+
	                        |
				+-- ListViewContainer -+
				                       |
						       +-- ListViewCol
						       |
						       +-- ListViewCol
						       |
						       +-- ListViewCol


  +-----------------------------------------------------+
  |                      ListView                       |
  |+---------------------------------------------------+|
  ||               |  ListViewHeader      |            ||
  || +-----------------------------------------------+ ||
  || |               ListViewContainer               | ||
  || | +-------------++-------------++-------------+ | ||
  || | | ListViewCol || ListViewCol || ListViewCol | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | ^ ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | - ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | |             ||             ||             | | ||
  || | +-------------++-------------++-------------+ | ||
  || +-----------------------------------------------+ ||
  |+------<------------------------------------->------+|
  +-----------------------------------------------------+
 */
 

namespace os
{
	class ListViewHeader;
	class ListView::Private {
		public:
	    ListViewContainer* m_pcMainView;
	    ListViewHeader*    m_pcHeaderView;
	    ScrollBar*	       m_pcVScroll;
	    ScrollBar*	       m_pcHScroll;
	    Message*	       m_pcSelChangeMsg;
	    Message*	       m_pcInvokeMsg;
	    uint32 m_nModeFlags;
	
		Private() {
			m_pcMainView = NULL;
			m_pcHeaderView = NULL;
			m_pcVScroll = NULL;
			m_pcHScroll = NULL;
			m_pcSelChangeMsg = NULL;
			m_pcInvokeMsg = NULL;
		}
		~Private() {
			delete m_pcSelChangeMsg;
			delete m_pcInvokeMsg;
		}
	};

	class ListViewCol:public View
	{
	      public:
		ListViewCol( ListViewContainer * pcParent, const Rect & cFrame, const String & cTitle );
		~ListViewCol();

		virtual void Paint( const Rect & cUpdateRect );
		void Refresh( const Rect & cUpdateRect );

		virtual void MouseDown(const Point&, uint32);
		virtual void MouseMove(const Point&,int,uint32,Message*);
		virtual void MouseUp(const Point&,uint32, Message*);

	      private:
		  friend class ListViewHeader;
		friend class ListViewContainer;

		ListViewContainer *m_pcParent;
		  String m_cTitle;
		float m_vContentWidth;
	};

	class ListViewHeader:public View
	{
		friend class ListView;
		friend class ListViewRow;
		friend class ListViewCol;

		  ListViewHeader( ListView * pcParent, const Rect & cFrame, uint32 nModeFlags );
		void DrawButton( const char *pzTitle, const Rect & cFrame, Font * pcFont, font_height * psFontHeight );
		virtual void Paint( const Rect & cUpdateRect );

		virtual void MouseDown( const Point & cPosition, uint32 nButton );
		virtual void MouseUp( const Point & cPosition, uint32 nButton, Message * pcData );
		virtual void MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData );
		virtual void FrameSized( const Point & cDelta );
		virtual void ViewScrolled( const Point & cDelta );

		virtual bool HasFocus( void ) const;

		void Layout();

		ListView *m_pcParent;
		ListViewContainer *m_pcMainView;

		int m_nSizeColumn;
		int m_nDragColumn;
		Point m_cHitPos;
		float m_vHeaderHeight;
	};

	class ListViewContainer:public View
	{
		friend class ListView;
		friend class ListViewHeader;
		friend class ListViewRow;
		friend class ListViewCol;

		enum
		{ AUTOSCROLL_TIMER = 1 };

		  ListViewContainer( ListView * pcListView, const Rect & cFrame, uint32 nModeFlags );
		  ~ListViewContainer();
		int InsertColumn( const char *pzTitle, int nWidth, int nPos = -1 );
		int InsertRow( int nPos, ListViewRow * pcRow, bool bUpdate );
		ListViewRow *RemoveRow( int nIndex, bool bUpdate );
		void InvalidateRow( int nRow, uint32 nFlags, bool bImidiate = false );
		void Clear();
		int GetRowIndex( float y ) const;
		void MakeVisible( int nRow, bool bCenter );
		void StartScroll( ListView::scroll_direction eDirection, bool bSelect );
		void StopScroll();
		void LayoutRows();

		virtual void FrameSized( const Point & cDelta );
		virtual void MouseDown( const Point & cPosition, uint32 nButton );
		virtual void MouseUp( const Point & cPosition, uint32 nButton, Message * pcData );
		virtual void MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData );
		virtual void WheelMoved( const Point & cDelta );
		virtual void Paint( const Rect & cUpdateRect );
		virtual void TimerTick( int nID );
		virtual void DetachedFromWindow( void );
		virtual bool HasFocus( void ) const;

		bool HandleKey( char nChar, uint32 nQualifiers );
		void LayoutColumns();
		ListViewCol *GetColumn( int nCol ) const
		{
			return ( m_cCols[m_cColMap[nCol]] );
		}


		bool ClearSelection();
		bool SelectRange( int nStart, int nEnd, bool bSelect );
		void InvertRange( int nStart, int nEnd );
		void ExpandSelect( int nNumRow, bool bInvert, bool bClear );

		  std::vector < ListViewCol * >m_cCols;
		  std::vector < int >m_cColMap;
		  std::vector < ListViewRow * >m_cRows;
		ListView *m_pcListView;
		uint32 m_nModeFlags;
		Rect m_cSelectRect;
		bool m_bIsSelecting;
		bool m_bMouseMoved;
		bool m_bDragIfMoved;
		int m_nBeginSel;
		int m_nEndSel;
		bigtime_t m_nMouseDownTime;
		int m_nLastHitRow;
		int m_nFirstSel;
		int m_nLastSel;
		float m_vVSpacing;
		float m_vTotalWidth;	// Total with of columns
		float m_vContentHeight;
		int m_nSortColumn;
		bool m_bAutoScrollUp;
		bool m_bAutoScrollDown;
		bool m_bMousDownSeen;
		bool m_bAutoScrollSelects;
	};

	class DummyRow:public ListViewRow
	{
	      public:
		DummyRow()
		{
		}

		virtual void AttachToView( View * pcView, int nColumn )
		{
		}
		virtual void SetRect( const Rect & cRect, int nColumn )
		{
		}

		virtual float GetWidth( View * pcView, int nColumn )
		{
			return ( 0.0f );
		}
		virtual float GetHeight( View * pcView )
		{
			return ( 0.0f );
		}
		virtual void Paint( const Rect & cFrame, View * pcView, uint nColumn, bool bSelected, bool bHighlighted, bool bHasFocus )
		{
		};
		virtual bool IsLessThan( const ListViewRow * pcOther, uint nColumn ) const
		{
			return ( false );
		}

	};

}				// end of namespace


ListViewRow::ListViewRow()
{
	m_vHeight = 0.0f;
	m_nFlags = F_SELECTABLE | F_VISIBLE;
}

ListViewRow::~ListViewRow()
{

}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ListViewRow::HitTest( View * pcView, const Rect & cFrame, int nColumn, Point cPos )
{
	return ( true );
}

void ListViewRow::SetCookie( Variant cCookie )
{
	m_cCookie = cCookie;
}

Variant ListViewRow::GetCookie( void )
{
	return m_cCookie;
}

void ListViewRow::SetIsSelectable( bool bSelectable )
{
	m_nFlags &= ~F_SELECTABLE;
	if( bSelectable )
		m_nFlags |= F_SELECTABLE;
}

bool ListViewRow::IsSelectable() const
{
	return ( m_nFlags & F_SELECTABLE );
}

bool ListViewRow::IsSelected() const
{
	return ( m_nFlags & F_SELECTED );
}

bool ListViewRow::IsHighlighted() const
{
	return ( m_nFlags & F_HIGHLIGHTED );
}
void ListViewRow::SetIsVisible( bool bVisible )
{
	m_nFlags &= ~F_VISIBLE;
	if( bVisible )
		m_nFlags |= F_VISIBLE;
}

bool ListViewRow::IsVisible() const
{
	return ( m_nFlags & F_VISIBLE );
}

void ListViewRow::SetHighlighted( bool bHigh )
{
	m_nFlags &= ~F_HIGHLIGHTED;
	if( bHigh )
		m_nFlags |= F_HIGHLIGHTED;
}

void ListViewRow::SetSelected( bool bSel )
{
	m_nFlags &= ~F_SELECTED;
	if( bSel )
		m_nFlags |= F_SELECTED;
}

void ListViewRow::__LVR_reserved_1__()
{
}

void ListViewRow::__LVR_reserved_2__()
{
}

void ListViewStringRow::__LVSR_reserved_1__()
{
}

void ListViewStringRow::__LVSR_reserved_2__()
{
}

void ListViewStringRow::AttachToView( View * pcView, int nColumn )
{
	m_cStrings[nColumn].second = pcView->GetStringWidth( m_cStrings[nColumn].first ) + 5.0f;
}

void ListViewStringRow::SetRect( const Rect & cRect, int nColumn )
{
}

void ListViewStringRow::AppendString( const String & cString )
{
	m_cStrings.push_back( std::make_pair( cString, 0.0f ) );
}

void ListViewStringRow::SetString( int nIndex, const String & cString )
{
	m_cStrings[nIndex].first = cString;
}

const String & ListViewStringRow::GetString( int nIndex ) const
{
	return ( m_cStrings[nIndex].first );
}



//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float ListViewStringRow::GetWidth( View * pcView, int nIndex )
{
	return ( m_cStrings[nIndex].second );
//    return( pcView->GetStringWidth( m_cStrings[nIndex] ) + 5.0f );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float ListViewStringRow::GetHeight( View * pcView )
{
	font_height sHeight;

	pcView->GetFontHeight( &sHeight );

	return ( sHeight.ascender + sHeight.descender );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewStringRow::Paint( const Rect & cFrame, View * pcView, uint nColumn, bool bSelected, bool bHighlighted, bool bHasFocus )
{
	font_height sHeight;

	pcView->GetFontHeight( &sHeight );

	pcView->SetFgColor( 255, 255, 255 );
	pcView->FillRect( cFrame );

	float vFontHeight = sHeight.ascender + sHeight.descender;
	float vBaseLine = cFrame.top + ( cFrame.Height() + 1.0f ) / 2 - vFontHeight / 2 + sHeight.ascender;

		
	if( bSelected || bHighlighted )
	{
		Rect cSelectFrame = cFrame;
		if( nColumn == 0 ) {
			cSelectFrame.left += 2;
			cSelectFrame.top += 2;
			cSelectFrame.bottom -= 2;
		}
		if( bSelected )
			pcView->SetFgColor( 186, 199, 227 );
		else
			pcView->SetFgColor( 0, 50, 200 );
		pcView->FillRect( cSelectFrame );
	
		/* Round edges */
		if( nColumn == 0 )
		{
			pcView->DrawLine( os::Point( cSelectFrame.left + 2, cSelectFrame.top - 2 ), 
								os::Point( cSelectFrame.right, cSelectFrame.top - 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left, cSelectFrame.top - 1 ), 
								os::Point( cSelectFrame.right, cSelectFrame.top - 1 ) );
			
			pcView->DrawLine( os::Point( cSelectFrame.left - 2, cSelectFrame.top + 2 ), 
								os::Point( cSelectFrame.left - 2, cSelectFrame.bottom - 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left - 1, cSelectFrame.top ), 
								os::Point( cSelectFrame.left - 1, cSelectFrame.bottom ) );
								
			pcView->DrawLine( os::Point( cSelectFrame.left + 2, cSelectFrame.bottom + 2 ), 
								os::Point( cSelectFrame.right, cSelectFrame.bottom + 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left, cSelectFrame.bottom + 1 ), 
								os::Point( cSelectFrame.right, cSelectFrame.bottom + 1 ) );
		} 
	}

	if( bHighlighted )
	{
		pcView->SetFgColor( 255, 255, 255 );
		pcView->SetBgColor( 0, 50, 200 );
	}
	else if( bSelected )
	{
		pcView->SetFgColor( 0, 0, 0 );
		pcView->SetBgColor( 186, 199, 227 );
	}
	else
	{
		pcView->SetBgColor( 255, 255, 255 );
		pcView->SetFgColor( 0, 0, 0 );
	}

	pcView->MovePenTo( cFrame.left + 3.0f, vBaseLine );
	pcView->DrawString( m_cStrings[nColumn].first );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ListViewStringRow::IsLessThan( const ListViewRow * pcOther, uint nColumn ) const
{
	const ListViewStringRow *pcRow = dynamic_cast < const ListViewStringRow * >( pcOther );

	if( NULL == pcRow || nColumn >= m_cStrings.size() || nColumn >= pcRow->m_cStrings.size(  ) )
	{
		return ( false );
	}
	return ( m_cStrings[nColumn].first < pcRow->m_cStrings[nColumn].first );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListViewCol::ListViewCol( ListViewContainer * pcParent, const Rect & cFrame, const String & cTitle ):
View( cFrame, "_lv_column" ), m_cTitle( cTitle )
{
	m_pcParent = pcParent;
	m_vContentWidth = 0.0f;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListViewCol::~ListViewCol()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
void ListViewCol::Refresh( const Rect & cUpdateRect )
{
	if( m_pcParent->m_cRows.empty() )
	{
		SetFgColor( 255, 255, 255 );
		FillRect( cUpdateRect );
		return;
	}

	Rect cBounds = GetBounds();

	std::vector < ListViewRow * >&cList = m_pcParent->m_cRows;
	ListViewRow *pcLastRow = m_pcParent->m_cRows[0];

	// Find the last visible row
	for( int i = 0; i < int ( cList.size() ); ++i )
	{
		if( cList[i]->IsVisible() )
		{
			pcLastRow = cList[i];
		}
	}

	if( cUpdateRect.top > pcLastRow->m_vYPos + pcLastRow->m_vHeight + m_pcParent->m_vVSpacing )
	{
		SetFgColor( 255, 255, 255 );
		FillRect( cUpdateRect );
		return;
	}

	int nFirstRow = m_pcParent->GetRowIndex( cUpdateRect.top );

	if( nFirstRow < 0 )
	{
		nFirstRow = 0;
	}

	uint nColumn = 0;

	for( nColumn = 0; nColumn < m_pcParent->m_cCols.size(); ++nColumn )
	{
		if( m_pcParent->m_cCols[nColumn] == this )
		{
			break;
		}
	}

	bool bHasFocus = m_pcParent->m_pcListView->HasFocus();

	Rect cFrame( cBounds.left, 0.0f, cBounds.right, 0.0f );

	for( int i = nFirstRow; i < int ( cList.size() ); ++i )
	{
		if( cList[i]->IsVisible() )
		{
			if( cList[i]->m_vYPos > cUpdateRect.bottom )
			{
				break;
			}
			cFrame.top = cList[i]->m_vYPos;
			cFrame.bottom = cFrame.top + cList[i]->m_vHeight + m_pcParent->m_vVSpacing - 1.0f;

			cList[i]->Paint( cFrame, this, nColumn, cList[i]->IsSelected(), cList[i]->IsHighlighted(  ), bHasFocus );
		}
	}

	if( cFrame.bottom < cUpdateRect.bottom )
	{
		cFrame.top = cFrame.bottom + 1.0f;
		cFrame.bottom = cUpdateRect.bottom;
		SetFgColor( 255, 255, 255 );
		FillRect( cFrame );
	}
}

//----------------------------------------------------------------------------
// Name:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewCol::Paint( const Rect & cUpdateRect )
{
	if( cUpdateRect.IsValid() == false )
	{
		return;		// FIXME: Workaround for appserver bug. Fix appserver.
	}
	if( m_pcParent->m_bIsSelecting )
	{
		m_pcParent->SetDrawingMode( DM_INVERT );
		m_pcParent->DrawFrame( m_pcParent->m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		m_pcParent->SetDrawingMode( DM_COPY );
	}
	Refresh( cUpdateRect );
	if( m_pcParent->m_bIsSelecting )
	{
		m_pcParent->SetDrawingMode( DM_INVERT );
		m_pcParent->DrawFrame( m_pcParent->m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		m_pcParent->SetDrawingMode( DM_COPY );
	}
}

void ListViewCol::MouseDown(const Point& cPos, uint32 nButtons)
{
	m_pcParent->MouseDown( cPos + GetFrame().LeftTop(), nButtons );
}

void ListViewCol::MouseUp(const Point& cPos, uint32 nButtons, Message* pcData)
{
	m_pcParent->MouseUp( cPos + GetFrame().LeftTop(), nButtons, pcData );
}

void ListViewCol::MouseMove(const Point& cPos,int nCode,uint32 nButtons, Message* pcData)
{
	m_pcParent->MouseMove( cPos + GetFrame().LeftTop(), nCode, nButtons, pcData );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListViewContainer::ListViewContainer( ListView * pcListView, const Rect & cFrame, uint32 nModeFlags ):View( cFrame, "main_view", CF_FOLLOW_NONE, WID_WILL_DRAW | WID_DRAW_ON_CHILDREN )
{
	m_pcListView = pcListView;
	m_vVSpacing = 3.0f;
	m_nSortColumn = 0;
	m_vTotalWidth = 0.0f;
	m_vContentHeight = 0.0f;
	m_nBeginSel = -1;
	m_nEndSel = -1;
	m_nFirstSel = -1;
	m_nLastSel = -1;
	m_nMouseDownTime = 0;
	m_nLastHitRow = -1;
	m_bMousDownSeen = false;
	m_bDragIfMoved = false;
	m_bAutoScrollSelects = false;
	m_bIsSelecting = false;
	m_nModeFlags = nModeFlags;
}

ListViewContainer::~ListViewContainer()
{
	for( uint i = 0; i < m_cRows.size(); ++i )
	{
		delete m_cRows[i];
	}
	m_cRows.clear();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ListViewContainer::HasFocus( void ) const
{
	if( View::HasFocus() )
	{
		return ( true );
	}
	for( uint i = 0; i < m_cColMap.size(); ++i )
	{
		if( m_cCols[m_cColMap[i]]->HasFocus() )
		{
			return ( true );
		}
	}
	return ( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::DetachedFromWindow( void )
{
	StopScroll();
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::StartScroll( ListView::scroll_direction eDirection, bool bSelect )
{
	m_bAutoScrollSelects = bSelect;

	if( eDirection == ListView::SCROLL_DOWN )
	{
		if( m_bAutoScrollDown == false )
		{
			m_bAutoScrollDown = true;
			if( m_bAutoScrollUp == false )
			{
				Looper *pcLooper = GetLooper();

				if( pcLooper != NULL )
				{
					pcLooper->AddTimer( this, AUTOSCROLL_TIMER, 50000, false );
				}
			}
			else
			{
				m_bAutoScrollUp = false;
			}
		}
	}
	else
	{
		if( m_bAutoScrollUp == false )
		{
			m_bAutoScrollUp = true;
			if( m_bAutoScrollDown == false )
			{
				Looper *pcLooper = GetLooper();

				if( pcLooper != NULL )
				{
					pcLooper->AddTimer( this, AUTOSCROLL_TIMER, 50000, false );
				}
			}
			else
			{
				m_bAutoScrollDown = false;
			}
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::StopScroll()
{
	if( m_bAutoScrollUp || m_bAutoScrollDown )
	{
		m_bAutoScrollUp = m_bAutoScrollDown = false;
		Looper *pcLooper = GetLooper();

		if( pcLooper != NULL )
		{
			pcLooper->RemoveTimer( this, AUTOSCROLL_TIMER );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::MouseDown( const Point & cPosition, uint32 nButton )
{
	MakeFocus( true );

	if( !m_cRows.empty() )
	{
		m_bMousDownSeen = true;

		int nHitCol = -1;
		int nHitRow;

		if( cPosition.y >= m_vContentHeight )
		{
			nHitRow = m_cRows.size() - 1;
		}
		else
		{
			nHitRow = GetRowIndex( cPosition.y );
	//              cout << "got row index: " << nHitRow << endl;
			if( nHitRow < 0 )
			{
				nHitRow = 0;
			}
		}

		bool bDoubleClick = false;

		bigtime_t nCurTime = get_system_time();

		if( nHitRow == m_nLastHitRow && nCurTime - m_nMouseDownTime < 500000 )
		{
			bDoubleClick = true;
		}
		m_nLastHitRow = nHitRow;

		ListViewRow *pcHitRow = ( nHitRow >= 0 ) ? m_cRows[nHitRow] : NULL;

		m_nMouseDownTime = nCurTime;

		uint32 nQualifiers = Application::GetInstance()->GetQualifiers(  );

		if( ( nQualifiers & QUAL_SHIFT ) && ( m_nModeFlags & ListView::F_MULTI_SELECT ) )
		{
			m_nEndSel = m_nBeginSel;
		}
		else
		{
			m_nBeginSel = nHitRow;
			m_nEndSel = nHitRow;
		}

		if( bDoubleClick )
		{
			m_pcListView->Invoked( m_nFirstSel, m_nLastSel );
			return;
		}

		if( pcHitRow != NULL )
		{
			for( uint i = 0; i < m_cColMap.size(); ++i )
			{
				Rect cFrame = GetColumn( i )->GetFrame();
	
				if( cPosition.x >= cFrame.left && cPosition.x < cFrame.right )
				{
					Rect cFrame = GetColumn( i )->GetFrame();
	
					cFrame.top = pcHitRow->m_vYPos;
					cFrame.bottom = cFrame.top + pcHitRow->m_vHeight + m_vVSpacing;
					Point cPos = cPosition - cFrame.LeftTop();
					if( pcHitRow->HitTest( GetColumn( i ), cFrame, i, cPos ) )
					{
						nHitCol = i;
					}
					break;
				}
			}
		}
		m_bMouseMoved = false;

		if( nHitCol == 1 && ( nQualifiers & QUAL_CTRL ) == 0 && m_cRows[nHitRow]->IsSelected() )
		{
			m_bDragIfMoved = true;
		}
		else
		{
			ExpandSelect( nHitRow, ( nQualifiers & QUAL_CTRL ), ( ( nQualifiers & ( QUAL_CTRL | QUAL_SHIFT ) ) == 0 || ( m_nModeFlags & ListView::F_MULTI_SELECT ) == 0 ) );
			if( ( m_nModeFlags & ListView::F_MULTI_SELECT ) && ( MOUSE_BUT_LEFT == nButton ) )
			{
				m_bIsSelecting = true;
				m_cSelectRect = Rect( cPosition.x, cPosition.y, cPosition.x, cPosition.y );
				SetDrawingMode( DM_INVERT );
				DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
				SetDrawingMode( DM_COPY );
			}
		}
	}
	m_pcListView->MouseDown( ConvertToParent( cPosition ), nButton);
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::MouseUp( const Point & cPosition, uint32 nButton, Message * pcData )
{
	m_bMousDownSeen = false;

	Looper *pcLooper = GetLooper();

	if( pcLooper != NULL )
	{
		pcLooper->RemoveTimer( this, AUTOSCROLL_TIMER );
	}

	if( m_bDragIfMoved )
	{
		ExpandSelect( m_nEndSel, false, true );
		m_bDragIfMoved = false;
	}

	if( m_bIsSelecting )
	{
		SetDrawingMode( DM_INVERT );
		DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		SetDrawingMode( DM_COPY );
		m_bIsSelecting = false;
		Flush();
	}
	if( pcData != NULL )
	{
		View::MouseUp( cPosition, nButton, pcData );
		return;
	}
	m_pcListView->MouseUp(cPosition,nButton,pcData);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	m_bMouseMoved = true;
	if( pcData != NULL )
	{
		View::MouseMove( cNewPos, nCode, nButtons, pcData );
		return;
	}
	if( m_bMousDownSeen == false )
	{
		return;
	}
	if( m_bDragIfMoved )
	{
		m_bDragIfMoved = false;
		if( m_pcListView->DragSelection( m_pcListView->ConvertFromScreen( ConvertToScreen( cNewPos ) ) ) )
		{
			m_bMousDownSeen = false;
			if( m_bIsSelecting )
			{
				SetDrawingMode( DM_INVERT );
				DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
				SetDrawingMode( DM_COPY );
				m_bIsSelecting = false;
			}
			return;
		}
	}

	if( m_bIsSelecting )
	{
		SetDrawingMode( DM_INVERT );
		DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		SetDrawingMode( DM_COPY );
		m_cSelectRect.right = cNewPos.x;
		m_cSelectRect.bottom = cNewPos.y;

		int nHitRow;

		if( cNewPos.y >= m_vContentHeight )
		{
			nHitRow = m_cRows.size() - 1;
		}
		else
		{
			nHitRow = GetRowIndex( cNewPos.y );
			if( nHitRow < 0 )
			{
				nHitRow = 0;
			}
		}
		if( nHitRow != m_nEndSel )
		{
			ExpandSelect( nHitRow, true, false );
		}
		SetDrawingMode( DM_INVERT );
		DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		SetDrawingMode( DM_COPY );
		Flush();
		Rect cBounds = GetBounds();

		if( cNewPos.y < cBounds.top + ListView::AUTOSCROLL_BORDER )
		{
			StartScroll( ListView::SCROLL_DOWN, true );
		}
		else if( cNewPos.y > cBounds.bottom - ListView::AUTOSCROLL_BORDER )
		{
			StartScroll( ListView::SCROLL_UP, true );
		}
		else
		{
			StopScroll();
		}
	}
	m_pcListView->MouseMove( cNewPos, nCode, nButtons, pcData );
}

void ListViewContainer::WheelMoved( const Point & cDelta )
{
	if( GetVScrollBar() != NULL )
	{
		GetVScrollBar()->WheelMoved( cDelta );
	}
	else
	{
		View::WheelMoved( cDelta );
	}
}

/*
int ListViewContainer::GetRowIndex( float y ) const
{
    if ( y < 0.0f || m_cRows.empty() ) {
	return( -1 );
    }
    
    DummyRow cDummy;
    cDummy.m_vYPos = y;

    std::vector<ListViewRow*>::const_iterator cIterator = std::lower_bound( m_cRows.begin(),
									    m_cRows.end(),
									    &cDummy,
									    RowPosPred() );
    
    int nIndex = cIterator - m_cRows.begin() - 1;
    if ( nIndex >= 0 && y > m_cRows[nIndex]->m_vYPos + m_cRows[nIndex]->m_vHeight + m_vVSpacing ) {
	nIndex = -1;
    }
    return( nIndex );
}
*/
int ListViewContainer::GetRowIndex( float y ) const
{
	int nIndex = -1, i = 0;

	if( y < 0.0f || m_cRows.empty() )
	{
		return ( -1 );
	}

	std::vector < ListViewRow * >::const_iterator cIterator;

	for( cIterator = m_cRows.begin(); cIterator != m_cRows.end(  ); cIterator++, i++ )
	{
		if( ( *cIterator )->IsVisible() && y < ( *cIterator )->m_vYPos + ( *cIterator )->m_vHeight + m_vVSpacing )
		{
			nIndex = i; //cIterator - m_cRows.begin();
			break;
		}
	}

	if( nIndex >= 0 && y > m_cRows[nIndex]->m_vYPos + m_cRows[nIndex]->m_vHeight + m_vVSpacing )
	{
		nIndex = -1;
	}
	return ( nIndex );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::FrameSized( const Point & cDelta )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::TimerTick( int nID )
{
	if( nID != AUTOSCROLL_TIMER )
	{
		View::TimerTick( nID );
		return;
	}

	Rect cBounds = GetBounds();

	if( cBounds.Height() >= m_vContentHeight )
	{
		return;
	}
	Point cMousePos;

	GetMouse( &cMousePos, NULL );

	float vPrevScroll = GetScrollOffset().y;
	float vCurScroll = vPrevScroll;

	if( m_bAutoScrollDown )
	{
		float nScrollStep = ( cBounds.top + ListView::AUTOSCROLL_BORDER ) - cMousePos.y;

		nScrollStep /= 2.0f;
		nScrollStep++;

		vCurScroll += nScrollStep;

		if( vCurScroll > 0 )
		{
			vCurScroll = 0;
		}
	}
	else if( m_bAutoScrollUp )
	{
		float vMaxScroll = -( m_vContentHeight - GetBounds().Height(  ) );

		float vScrollStep = cMousePos.y - ( cBounds.bottom - ListView::AUTOSCROLL_BORDER );

		vScrollStep *= 0.5f;
		vScrollStep = vScrollStep + 1.0f;

		vCurScroll -= vScrollStep;
		cMousePos.y += vScrollStep;

		if( vCurScroll < vMaxScroll )
		{
			vCurScroll = vMaxScroll;
		}
	}
	if( vCurScroll != vPrevScroll )
	{
		if( m_bIsSelecting )
		{
			SetDrawingMode( DM_INVERT );
			DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
			SetDrawingMode( DM_COPY );
		}
		ScrollTo( 0, vCurScroll );
		if( m_bIsSelecting )
		{
			m_cSelectRect.right = cMousePos.x;
			m_cSelectRect.bottom = cMousePos.y;

			int nHitRow;

			if( cMousePos.y >= m_vContentHeight )
			{
				nHitRow = m_cRows.size() - 1;
			}
			else
			{
				nHitRow = GetRowIndex( cMousePos.y );
				if( nHitRow < 0 )
				{
					nHitRow = 0;
				}
			}

			if( nHitRow != m_nEndSel )
			{
				ExpandSelect( nHitRow, true, false );
			}
			SetDrawingMode( DM_INVERT );
			DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
			SetDrawingMode( DM_COPY );
		}
		Flush();
	}
}

/** Handle keystrokes
 *	\par	Description:
 *			Handle pressed keys (called by ListView::KeyDown). Returns false
 *			if the key was not handled, in which case the key event is passed
 *			on to ListView::KeyDown().
 * \sa ListView::KeyDown()
 * \author Kurt Skauen, Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
bool ListViewContainer::HandleKey( char nChar, uint32 nQualifiers )
{
	if( m_cRows.empty() )
	{
		return ( false );
	}
	switch ( nChar )
	{
	case 0:
		return ( false );
	case VK_PAGE_UP:
	case VK_UP_ARROW:
		{
			if( m_nEndSel == 0 )
			{
				return ( true );
			}

			if( m_nEndSel == -1 )
			{
				m_nEndSel = m_nBeginSel;
			}
			if( m_nBeginSel == -1 )
			{
				// No item was selected, we'll select the last one
				m_nEndSel = m_cRows.size() - 1;

				// Iterate backwards through the list until a visible item is found
				for( ; m_nEndSel > 0; m_nEndSel-- )
				{
					if( m_cRows[m_nEndSel]->IsVisible() )
						break;
				}
				m_nBeginSel = m_nEndSel;

				ExpandSelect( m_nEndSel, false, ( nQualifiers & QUAL_SHIFT ) == 0 || ( m_nModeFlags & ListView::F_MULTI_SELECT ) == 0 );
			}
			else
			{
				if( ( nQualifiers & QUAL_SHIFT ) == 0 || ( m_nModeFlags & ListView::F_MULTI_SELECT ) == 0 )
				{
					m_nBeginSel = -1;
				}

				float vPageBreak = m_cRows[m_nEndSel]->m_vYPos - GetBounds().Height(  );
				int nNewSel = m_nEndSel - 1;

				for( ; nNewSel > 0; nNewSel-- )
				{
					if( m_cRows[nNewSel]->IsVisible() )
					{
						if( nChar == VK_UP_ARROW )
						{
							break;
						}
						else
						{
							if( m_cRows[nNewSel]->m_vYPos < vPageBreak )
							{
								break;
							}
						}
					}
				}
				ExpandSelect( nNewSel, false, ( nQualifiers & QUAL_SHIFT ) == 0 || ( m_nModeFlags & ListView::F_MULTI_SELECT ) == 0 );
			}
			if( ( nQualifiers & QUAL_SHIFT ) == 0 )
			{
				m_nBeginSel = m_nEndSel;
			}
			if( m_cRows[m_nEndSel]->m_vYPos < GetBounds().top )
			{
				MakeVisible( m_nEndSel, false );
			}
			Flush();
			return ( true );
		}
	case VK_PAGE_DOWN:
	case VK_DOWN_ARROW:
		{
			if( m_nEndSel == int ( m_cRows.size() ) - 1 )
			{
				return ( true );
			}

			if( m_nEndSel == -1 )
			{
				m_nEndSel = m_nBeginSel;
			}
			if( m_nBeginSel == -1 )
			{
				// No item was selected, we'll select the first one
				m_nEndSel = 0;

				// Iterate through the list until a visible item is found
				for( ; m_nEndSel < (int)m_cRows.size(); m_nEndSel++ )
				{
					if( m_cRows[m_nEndSel]->IsVisible() )
						break;
				}
				m_nBeginSel = m_nEndSel;

				ExpandSelect( m_nEndSel, false, ( nQualifiers & QUAL_SHIFT ) == 0 || ( m_nModeFlags & ListView::F_MULTI_SELECT ) == 0 );
			}
			else
			{
				if( ( nQualifiers & QUAL_SHIFT ) == 0 || ( m_nModeFlags & ListView::F_MULTI_SELECT ) == 0 )
				{
					m_nBeginSel = -1;
				}

				float vPageBreak = m_cRows[m_nEndSel]->m_vYPos + GetBounds().Height(  );
				int nNewSel = m_nEndSel + 1;

				for( ; nNewSel < (int)m_cRows.size(); nNewSel++ )
				{
					if( m_cRows[nNewSel]->IsVisible() )
					{
						if( nChar == VK_DOWN_ARROW )
						{
							break;
						}
						else
						{
							if( m_cRows[nNewSel]->m_vYPos + m_cRows[m_nEndSel]->m_vHeight > vPageBreak )
							{
								break;
							}
						}
					}
				}
				ExpandSelect( nNewSel, false, ( nQualifiers & QUAL_SHIFT ) == 0 || ( m_nModeFlags & ListView::F_MULTI_SELECT ) == 0 );
			}
			if( ( nQualifiers & QUAL_SHIFT ) == 0 )
			{
				m_nBeginSel = m_nEndSel;
			}
			if( m_cRows[m_nEndSel]->m_vYPos + m_cRows[m_nEndSel]->m_vHeight + m_vVSpacing > GetBounds().bottom )
			{
				MakeVisible( m_nEndSel, false );
			}
			Flush();
			return ( true );
		}
	case '\n':
		if( m_nFirstSel >= 0 )
		{
			m_pcListView->Invoked( m_nFirstSel, m_nLastSel );
		}
		return ( true );
	default:
		return ( false );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::MakeVisible( int nRow, bool bCenter )
{
	float vTotRowHeight = m_cRows[nRow]->m_vHeight + m_vVSpacing;
	float y = m_cRows[nRow]->m_vYPos;
	Rect cBounds = GetBounds();
	float vViewHeight = cBounds.Height() + 1.0f;

	if( m_vContentHeight <= vViewHeight )
	{
		ScrollTo( 0, 0 );
	}
	else
	{
		if( bCenter )
		{
			float vOffset = y - vViewHeight * 0.5f + vTotRowHeight * 0.5f;

			if( vOffset < 0.0f )
			{
				vOffset = 0.0f;
			}
			else if( vOffset >= m_vContentHeight - 1 )
			{
				vOffset = m_vContentHeight - 1;
			}
			if( vOffset < 0.0f )
			{
				vOffset = 0.0f;
			}
			else if( vOffset > m_vContentHeight - vViewHeight )
			{
				vOffset = m_vContentHeight - vViewHeight;
			}
			ScrollTo( 0, -vOffset );
		}
		else
		{
			float vOffset;

			if( y + vTotRowHeight * 0.5f < cBounds.top + vViewHeight * 0.5f )
			{
				vOffset = y;
			}
			else
			{
				vOffset = -( vViewHeight - ( y + vTotRowHeight ) );
			}
			if( vOffset < 0.0f )
			{
				vOffset = 0.0f;
			}
			else if( vOffset > m_vContentHeight - vViewHeight )
			{
				vOffset = m_vContentHeight - vViewHeight;
			}
			ScrollTo( 0, -vOffset );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::LayoutColumns()
{
	float x = 0;

	for( uint i = 0; i < m_cColMap.size(); ++i )
	{
		Rect cFrame = GetColumn( i )->GetFrame();

		GetColumn( i )->MoveTo( x, 0 );
		if( i == m_cColMap.size() - 1 )
		{
			x += GetColumn( i )->m_vContentWidth;
		}
		else
		{
			x += cFrame.Width() + 1.0f;
		}
	}
	m_vTotalWidth = x;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ListViewContainer::InsertColumn( const char *pzTitle, int nWidth, int nPos )
{
	ListViewCol *pcCol = new ListViewCol( this, Rect( 0, 0, nWidth - 1, 16000000.0f ), pzTitle );

	AddChild( pcCol );
	int nIndex;

	if( nPos == -1 )
	{
		nIndex = m_cCols.size();
		m_cCols.push_back( pcCol );
	}
	else
	{
		nIndex = nPos;
		m_cCols.insert( m_cCols.begin() + nPos, pcCol );
	}
	m_cColMap.push_back( nIndex );
	LayoutColumns();
	return ( nIndex );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------
int ListViewContainer::InsertRow( int nPos, ListViewRow * pcRow, bool bUpdate )
{
	int nIndex;

	if( nPos >= 0 )
	{
		if( nPos > int ( m_cRows.size() ) )
		{
			nPos = m_cRows.size();
		}
		m_cRows.insert( m_cRows.begin() + nPos, pcRow );
		nIndex = nPos;
	}
	else
	{
		if( ( m_nModeFlags & ListView::F_NO_AUTO_SORT ) == 0 && m_nSortColumn >= 0 )
		{
			std::vector < ListViewRow * >::iterator cIterator;

			cIterator = std::lower_bound( m_cRows.begin(), m_cRows.end(  ), pcRow, RowContentPred( m_nSortColumn ) );
			cIterator = m_cRows.insert( cIterator, pcRow );
			nIndex = cIterator - m_cRows.begin();
		}
		else
		{
			nIndex = m_cRows.size();
			m_cRows.push_back( pcRow );
		}
	}
	for( uint i = 0; i < m_cCols.size(); ++i )
	{
		pcRow->AttachToView( m_cCols[i], i );
	}
	for( uint i = 0; i < m_cCols.size(); ++i )
	{
		float vWidth = pcRow->GetWidth( m_cCols[i], i );

		if( vWidth > m_cCols[i]->m_vContentWidth )
		{
			m_cCols[i]->m_vContentWidth = vWidth;
		}
	}
	pcRow->m_vHeight = pcRow->GetHeight( this );

	m_vContentHeight += pcRow->m_vHeight + m_vVSpacing;

	for( uint i = nIndex; i < m_cRows.size(); ++i )
	{
		for( ; i < m_cRows.size(  ) && !m_cRows[i]->IsVisible(); i++ );
		if( i >= m_cRows.size() )
			break;

		float y = 0.0f;

		if( i > 0 )
		{
			y = m_cRows[i - 1]->m_vYPos + m_cRows[i - 1]->m_vHeight + m_vVSpacing;
		}
		m_cRows[i]->m_vYPos = y;

		Rect r;

		r.top = y;
		r.bottom = r.top + m_cRows[i]->m_vHeight + m_vVSpacing;
		for( uint j = 0; j < m_cCols.size(); ++j )
		{
			r.left = m_cCols[j]->GetBounds().left;
			r.right = m_cCols[j]->GetBounds().right;	//m_cCols[j]->m_vContentWidth;
			m_cRows[i]->SetRect( r, j );
		}
	}
	if( bUpdate )
	{
		Rect cBounds = GetBounds();

		if( pcRow->m_vYPos + pcRow->m_vHeight + m_vVSpacing <= cBounds.bottom )
		{
			if( m_nModeFlags & ListView::F_DONT_SCROLL )
			{
				cBounds.top = pcRow->m_vYPos;
				for( uint i = 0; i < m_cCols.size(); ++i )
				{
					m_cCols[i]->Invalidate( cBounds );
				}
			}
			else
			{
				cBounds.top = pcRow->m_vYPos + pcRow->m_vHeight + m_vVSpacing;
				cBounds.bottom += pcRow->m_vHeight + m_vVSpacing;
				for( uint i = 0; i < m_cCols.size(); ++i )
				{
					m_cCols[i]->ScrollRect( cBounds - Point( 0.0f, pcRow->m_vHeight + m_vVSpacing ), cBounds );
					m_cCols[i]->Invalidate( Rect( cBounds.left, pcRow->m_vYPos, cBounds.right, cBounds.top ) );
				}
			}
		}
	}
	return ( nIndex );
}

ListViewRow *ListViewContainer::RemoveRow( int nIndex, bool bUpdate )
{
	ListViewRow *pcRow = m_cRows[nIndex];

	m_cRows.erase( m_cRows.begin() + nIndex );

	m_vContentHeight -= pcRow->m_vHeight + m_vVSpacing;

	for( uint i = nIndex; i < m_cRows.size(); ++i )
	{
		float y = 0.0f;

		if( i > 0 )
		{
			y = m_cRows[i - 1]->m_vYPos + m_cRows[i - 1]->m_vHeight + m_vVSpacing;
		}
		m_cRows[i]->m_vYPos = y;
	}

	for( uint i = 0; i < m_cCols.size(); ++i )
	{
		m_cCols[i]->m_vContentWidth = 0.0f;
		for( uint j = 0; j < m_cRows.size(); ++j )
		{
			float vWidth = m_cRows[j]->GetWidth( m_cCols[i], i );

			if( vWidth > m_cCols[i]->m_vContentWidth )
			{
				m_cCols[i]->m_vContentWidth = vWidth;
			}
		}
	}

	if( bUpdate )
	{
		Rect cBounds = GetBounds();

		float vRowHeight = pcRow->m_vHeight + m_vVSpacing;
		float y = pcRow->m_vYPos;

		if( y + vRowHeight <= cBounds.bottom )
		{
			if( m_nModeFlags & ListView::F_DONT_SCROLL )
			{
				cBounds.top = y;
				for( uint i = 0; i < m_cCols.size(); ++i )
				{
					m_cCols[i]->Invalidate( cBounds );
				}
			}
			else
			{
				cBounds.top = y + vRowHeight;
				cBounds.bottom += vRowHeight;
				for( uint i = 0; i < m_cCols.size(); ++i )
				{
					m_cCols[i]->ScrollRect( cBounds, cBounds - Point( 0.0f, vRowHeight ) );
				}
			}
		}
	}
	bool bSelChanged = pcRow->IsSelected();

	if( m_nFirstSel != -1 )
	{
		if( m_nFirstSel == m_nLastSel && nIndex == m_nFirstSel )
		{
			m_nFirstSel = m_nLastSel = -1;
			bSelChanged = true;
		}
		else
		{
			if( nIndex < m_nFirstSel )
			{
				m_nFirstSel--;
				m_nLastSel--;
				bSelChanged = true;
			}
			else if( nIndex == m_nFirstSel )
			{
				m_nLastSel--;
				for( int i = nIndex; i <= m_nLastSel; ++i )
				{
					if( m_cRows[i]->IsSelected() )
					{
						m_nFirstSel = i;
						bSelChanged = true;
						break;
					}
				}
			}
			else if( nIndex < m_nLastSel )
			{
				m_nLastSel--;
				bSelChanged = true;
			}
			else if( nIndex == m_nLastSel )
			{
				for( int i = m_nLastSel - 1; i >= m_nFirstSel; --i )
				{
					if( m_cRows[i]->IsSelected() )
					{
						m_nLastSel = i;
						bSelChanged = true;
						break;
					}
				}
			}
		}
	}
	if( m_nBeginSel != -1 && m_nBeginSel >= int ( m_cRows.size() ) )
	{
		m_nBeginSel = m_cRows.size() - 1;
	}
	if( m_nEndSel != -1 && m_nEndSel >= int ( m_cRows.size() ) )
	{
		m_nEndSel = m_cRows.size() - 1;
	}
	if( bSelChanged )
	{
		m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
	}
	return ( pcRow );

}

void ListViewContainer::LayoutRows()
{
	float y = 0.0f;

	for( uint i = 0; i < m_cRows.size(); ++i )
	{
		if( m_cRows[i]->IsVisible() )
		{
			m_cRows[i]->m_vYPos = y;
			y = m_cRows[i]->m_vYPos + m_cRows[i]->m_vHeight + m_vVSpacing;
		}
	}

	m_vContentHeight = y;

	for( uint i = 0; i < m_cCols.size(); ++i )
	{
		m_cCols[i]->m_vContentWidth = 0.0f;
		for( uint j = 0; j < m_cRows.size(); ++j )
		{
			if( m_cRows[j]->IsVisible() )
			{
				float vWidth = m_cRows[j]->GetWidth( m_cCols[i], i );

				if( vWidth > m_cCols[i]->m_vContentWidth )
				{
					m_cCols[i]->m_vContentWidth = vWidth;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::Clear()
{
	if( m_bIsSelecting )
	{
		SetDrawingMode( DM_INVERT );
		DrawFrame( m_cSelectRect, FRAME_TRANSPARENT | FRAME_THIN );
		SetDrawingMode( DM_COPY );
		m_bIsSelecting = false;
		Flush();
	}
	m_bMouseMoved = true;
	m_bDragIfMoved = false;

	for( uint i = 0; i < m_cRows.size(); ++i )
	{
		delete m_cRows[i];
	}
	m_cRows.clear();
	m_vContentHeight = 0.0f;

	Rect cBounds = GetBounds();

	for( uint i = 0; i < m_cColMap.size(); ++i )
	{
		GetColumn( i )->Invalidate( cBounds );
	}
	for( uint i = 0; i < m_cCols.size(); ++i )
	{
		m_cCols[i]->m_vContentWidth = 0.0f;
	}
	m_nBeginSel = -1;
	m_nEndSel = -1;
	m_nFirstSel = -1;
	m_nLastSel = -1;
	m_nMouseDownTime = 0;
	m_nLastHitRow = -1;
	ScrollTo( 0, 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::Paint( const Rect & cUpdateRect )
{
	Rect cFrame = GetBounds();

	cFrame.top = 0.0f;

	if( m_cColMap.size() > 0 )
	{
		Rect cLastCol = GetColumn( m_cColMap.size() - 1 )->GetFrame(  );

		cFrame.left = cLastCol.right + 1.0f;
	}
	SetFgColor( 255, 255, 255 );
	cFrame.bottom = COORD_MAX;
	FillRect( cFrame );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewContainer::InvalidateRow( int nRow, uint32 nFlags, bool bImediate )
{
	float vTotRowHeight = m_cRows[nRow]->m_vHeight + m_vVSpacing;
	float y = m_cRows[nRow]->m_vYPos;
	Rect cFrame( 0.0f, y, COORD_MAX, y + vTotRowHeight - 1.0f );

	if( GetBounds().DoIntersect( cFrame ) == false )
	{
		return;
	}

	if( !m_cRows[nRow]->IsVisible() )
		return;

	if( bImediate )
	{
		bool bHasFocus = m_pcListView->HasFocus();

		for( uint i = 0; i < m_cColMap.size(); ++i )
		{
			Rect cColFrame( cFrame );

			cColFrame.right = GetColumn( i )->GetFrame().Width(  );
			m_cRows[nRow]->Paint( cColFrame, GetColumn( i ), i, m_cRows[nRow]->IsSelected(), m_cRows[nRow]->IsHighlighted(  ), bHasFocus );
		}
	}
	else
	{
		for( uint i = 0; i < m_cColMap.size(); ++i )
		{
			Rect cColFrame( cFrame );

			cColFrame.right = GetColumn( i )->GetFrame().Width(  );
			GetColumn( i )->Invalidate( cColFrame );
		}
	}
}

void ListViewContainer::InvertRange( int nStart, int nEnd )
{
	for( int i = nStart; i <= nEnd; ++i )
	{
		if( m_cRows[i]->IsSelectable() )
		{
			m_cRows[i]->SetSelected( !m_cRows[i]->IsSelected() );
			InvalidateRow( i, ListView::INV_VISUAL );
		}
	}
	if( m_nFirstSel == -1 )
	{
		m_nFirstSel = nStart;
		m_nLastSel = nEnd;
	}
	else
	{
		if( nEnd < m_nFirstSel )
		{
			for( int i = nStart; i <= nEnd; ++i )
			{
				if( m_cRows[i]->IsSelected() )
				{
					m_nFirstSel = i;
					break;
				}
			}
		}
		else if( nStart > m_nLastSel )
		{
			for( int i = nEnd; i >= nStart; --i )
			{
				if( m_cRows[i]->IsSelected() )
				{
					m_nLastSel = i;
					break;
				}
			}
		}
		else
		{
			if( nStart <= m_nFirstSel )
			{
				m_nFirstSel = m_nLastSel + 1;
				for( int i = nStart; i <= m_nLastSel; ++i )
				{
					if( m_cRows[i]->IsSelected() )
					{
						m_nFirstSel = i;
						break;
					}
				}
			}
			if( nEnd >= m_nLastSel )
			{
				m_nLastSel = m_nFirstSel - 1;
				if( m_nFirstSel < int ( m_cRows.size() ) )
				{
					for( int i = nEnd; i >= m_nFirstSel; --i )
					{
						if( m_cRows[i]->IsSelected() )
						{
							m_nLastSel = i;
							break;
						}
					}
				}
			}
		}
	}
	if( m_nLastSel < m_nFirstSel )
	{
		m_nFirstSel = -1;
		m_nLastSel = -1;
	}
}

bool ListViewContainer::SelectRange( int nStart, int nEnd, bool bSelect )
{
	bool bChanged = false;

	for( int i = nStart; i <= nEnd; ++i )
	{
		if( m_cRows[i]->IsSelectable() && m_cRows[i]->IsSelected(  ) != bSelect )
		{
			m_cRows[i]->SetSelected( bSelect );
			InvalidateRow( i, ListView::INV_VISUAL );
			bChanged = true;
		}
	}
	if( bChanged )
	{
		if( m_nFirstSel == -1 )
		{
			if( bSelect )
			{
				m_nFirstSel = nStart;
				m_nLastSel = nEnd;
			}
		}
		else
		{
			if( bSelect )
			{
				if( nStart < m_nFirstSel )
				{
					m_nFirstSel = nStart;
				}
				if( nEnd > m_nLastSel )
				{
					m_nLastSel = nEnd;
				}
			}
			else
			{
				if( nEnd >= m_nFirstSel )
				{
					int i = m_nFirstSel;

					m_nFirstSel = m_nLastSel + 1;
					for( ; i <= m_nLastSel; ++i )
					{
						if( m_cRows[i]->IsSelected() )
						{
							m_nFirstSel = i;
							break;
						}
					}
				}
				if( nStart <= m_nLastSel && m_nLastSel < int ( m_cRows.size() ) )
				{
					int i = m_nLastSel;

					m_nLastSel = m_nFirstSel - 1;
					for( ; i >= m_nFirstSel - 1; --i )
					{
						if( m_cRows[i]->IsSelected() )
						{
							m_nLastSel = i;
							break;
						}
					}
				}
			}
			if( m_nLastSel < m_nFirstSel )
			{
				m_nFirstSel = -1;
				m_nLastSel = -1;
			}
		}
	}
	return ( bChanged );
}

void ListViewContainer::ExpandSelect( int nRow, bool bInvert, bool bClear )
{
	if( m_cRows.empty() )
	{
		return;
	}
	if( nRow < 0 )
	{
		nRow = 0;
	}
	if( nRow >= int ( m_cRows.size() ) )
	{
		nRow = m_cRows.size() - 1;
	}

	if( bClear )
	{
		if( m_nFirstSel != -1 )
		{
			for( int i = m_nFirstSel; i <= m_nLastSel; ++i )
			{
				if( m_cRows[i]->IsSelected() && m_cRows[i]->IsVisible(  ) )
				{
					m_cRows[i]->SetSelected( false );
					InvalidateRow( i, ListView::INV_VISUAL, true );
				}
			}
		}
		m_nFirstSel = -1;
		m_nLastSel = -1;
		if( m_nBeginSel == -1 )
		{
			m_nBeginSel = nRow;
		}
		m_nEndSel = nRow;
		bool bChanged;

		if( m_nBeginSel < nRow )
		{
			bChanged = SelectRange( m_nBeginSel, nRow, true );
		}
		else
		{
			bChanged = SelectRange( nRow, m_nBeginSel, true );
		}
		if( bChanged )
		{
			m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
		}
		return;
	}
	if( m_nBeginSel == -1 || m_nEndSel == -1 )
	{
		m_nBeginSel = nRow;
		m_nEndSel = nRow;
		bool bSelChanged;

		if( bInvert )
		{
			InvertRange( nRow, nRow );
			bSelChanged = true;
		}
		else
		{
			bSelChanged = SelectRange( nRow, nRow, true );
		}
		if( bSelChanged )
		{
			m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
		}
		return;
	}
	if( nRow == m_nEndSel )
	{
		bool bSelChanged;

		if( bInvert )
		{
			InvertRange( nRow, nRow );
			bSelChanged = true;
		}
		else
		{
			bSelChanged = SelectRange( nRow, nRow, true );
		}
		if( bSelChanged )
		{
			m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
		}
		m_nEndSel = nRow;
		return;
	}
	bool bSelChanged = false;

	if( m_nBeginSel <= m_nEndSel )
	{
		if( nRow < m_nEndSel )
		{
			if( bInvert )
			{
				if( nRow < m_nBeginSel )
				{
					InvertRange( m_nBeginSel + 1, m_nEndSel );
					InvertRange( nRow, m_nBeginSel - 1 );
				}
				else
				{
					InvertRange( nRow + 1, m_nEndSel );
				}
				bSelChanged = true;
			}
			else
			{
				if( nRow < m_nBeginSel )
				{
					bSelChanged = SelectRange( m_nBeginSel + 1, m_nEndSel, false );
					bSelChanged = SelectRange( nRow, m_nBeginSel - 1, true ) || bSelChanged;
				}
				else
				{
					bSelChanged = SelectRange( nRow + 1, m_nEndSel, false );
				}
			}
		}
		else
		{
			if( bInvert )
			{
				InvertRange( m_nEndSel + 1, nRow );
				bSelChanged = true;
			}
			else
			{
				bSelChanged = SelectRange( m_nEndSel + 1, nRow, true );
			}
		}
	}
	else
	{
		if( nRow < m_nEndSel )
		{
			if( bInvert )
			{
				InvertRange( nRow, m_nEndSel - 1 );
				bSelChanged = true;
			}
			else
			{
				bSelChanged = SelectRange( nRow, m_nEndSel - 1, true );
			}
		}
		else
		{
			if( bInvert )
			{
				if( nRow > m_nBeginSel )
				{
					InvertRange( m_nEndSel, m_nBeginSel - 1 );
					InvertRange( m_nBeginSel + 1, nRow );
				}
				else
				{
					InvertRange( m_nEndSel, nRow - 1 );
				}
				bSelChanged = true;
			}
			else
			{
				if( nRow > m_nBeginSel )
				{
					bSelChanged = SelectRange( m_nEndSel, m_nBeginSel - 1, false );
					bSelChanged = SelectRange( m_nBeginSel + 1, nRow, true ) || bSelChanged;
				}
				else
				{
					bSelChanged = SelectRange( m_nEndSel, nRow - 1, false );
				}
			}
		}
	}
	if( bSelChanged )
	{
		m_pcListView->SelectionChanged( m_nFirstSel, m_nLastSel );
	}
	m_nEndSel = nRow;
}

bool ListViewContainer::ClearSelection()
{
	bool bChanged = false;

	if( m_nFirstSel != -1 )
	{
		for( int i = m_nFirstSel; i <= m_nLastSel; ++i )
		{
			if( m_cRows[i]->IsSelected() )
			{
				bChanged = true;
				m_cRows[i]->SetSelected( false );
				InvalidateRow( i, ListView::INV_VISUAL, true );
			}
		}
	}
	m_nFirstSel = -1;
	m_nLastSel = -1;
	return ( bChanged );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListViewHeader::ListViewHeader( ListView * pcParent, const Rect & cFrame, uint32 nModeFlags ):
View( cFrame, "header_view", CF_FOLLOW_LEFT | CF_FOLLOW_TOP, 0 )
{
	m_pcParent = pcParent;
	m_nSizeColumn = -1;
	m_nDragColumn = -1;
	m_pcMainView = new ListViewContainer( pcParent, cFrame, nModeFlags );
	AddChild( m_pcMainView );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ListViewHeader::HasFocus( void ) const
{
	if( View::HasFocus() )
	{
		return ( true );
	}
	return ( m_pcMainView->HasFocus() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::DrawButton( const char *pzTitle, const Rect & cFrame, Font * pcFont, font_height * psFontHeight )
{
	SetEraseColor( get_default_color( COL_LISTVIEW_TAB ) );
	DrawFrame( cFrame, FRAME_RAISED );

	SetFgColor( get_default_color( COL_LISTVIEW_TAB_TEXT ) );
	SetBgColor( get_default_color( COL_LISTVIEW_TAB ) );

	float vFontHeight = psFontHeight->ascender + psFontHeight->descender;

	int nStrLen = pcFont->GetStringLength( pzTitle, cFrame.Width() - 9.0f );

	MovePenTo(cFrame.LeftTop() + Point( 5, ( cFrame.Height(  ) + 1.0f ) / 2 - vFontHeight / 2 + psFontHeight->ascender ));

	DrawString( pzTitle, nStrLen );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::Paint( const Rect & cUpdateRect )
{
	Font *pcFont = GetFont();

	if( pcFont == NULL )
	{
		return;
	}

	font_height sHeight;

	pcFont->GetHeight( &sHeight );

	Rect cFrame;

	for( uint i = 0; i < m_pcMainView->m_cColMap.size(); ++i )
	{
		ListViewCol *pcCol = m_pcMainView->GetColumn( i );

		cFrame = pcCol->GetFrame();
		cFrame.top = 0;
		cFrame.bottom = sHeight.ascender + sHeight.descender + 6 - 1;
		if( i == m_pcMainView->m_cColMap.size() - 1 )
		{
			cFrame.right = COORD_MAX;
		}
		DrawButton( pcCol->m_cTitle.c_str(), cFrame, pcFont, &sHeight );
	}			/*
				   cFrame.left = cFrame.right + 1;
				   cFrame.right = GetBounds().right;
				   if ( cFrame.IsValid() ) {
				   DrawButton( "", cFrame, pcFont, &sHeight );
				   } */
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	if( pcData != NULL )
	{
		View::MouseMove( cNewPos, nCode, nButtons, pcData );
		return;
	}
	if( m_nDragColumn != -1 )
	{
		if( m_nDragColumn < int ( m_pcMainView->m_cColMap.size() ) )
		{
			Message cData( 0 );

			cData.AddInt32( "column", m_pcMainView->m_cColMap[m_nDragColumn] );
			Rect cFrame( m_pcMainView->GetColumn( m_nDragColumn )->GetFrame() );

//          if ( m_nDragColumn == int(m_pcMainView->m_cColMap.size()) - 1 ) {
//              cFrame.right = cFrame.left + m_pcMainView->GetColumn( m_nDragColumn )->m_vContentWidth;
//          }
			m_pcMainView->ConvertToParent( &cFrame );
			ConvertFromParent( &cFrame );
			cFrame.top = 0.0f;
			cFrame.bottom = m_vHeaderHeight - 1.0f;

			BeginDrag( &cData, cNewPos - cFrame.LeftTop() - GetScrollOffset(  ), cFrame.Bounds(  ), this );
		}
		m_nDragColumn = -1;
	}
	if( m_nSizeColumn == -1 )
	{
		return;
	}
	float vColWidth = m_pcMainView->GetColumn( m_nSizeColumn )->GetFrame().Width(  );
	float vDeltaSize = cNewPos.x - m_cHitPos.x;

	if( vColWidth + vDeltaSize < 4.0f )
	{
		vDeltaSize = 4.0f - vColWidth;
	}
	m_pcMainView->GetColumn( m_nSizeColumn )->ResizeBy( vDeltaSize, 0 );

	ListViewCol *pcCol = m_pcMainView->m_cCols[m_pcMainView->m_cColMap[m_pcMainView->m_cColMap.size() - 1]];
	Rect cFrame = pcCol->GetFrame();

	cFrame.right = m_pcParent->GetBounds().right - GetScrollOffset(  ).x;
	pcCol->SetFrame( cFrame );


	m_pcMainView->LayoutColumns();
	m_pcParent->AdjustScrollBars( false );
	Paint( GetBounds() );
	Flush();
	m_cHitPos = cNewPos;
}

void ListViewHeader::ViewScrolled( const Point & cDelta )
{
	if( m_pcMainView->m_cColMap.empty() )
	{
		return;
	}
	ListViewCol *pcCol = m_pcMainView->m_cCols[m_pcMainView->m_cColMap[m_pcMainView->m_cColMap.size() - 1]];
	Rect cFrame = pcCol->GetFrame();

	cFrame.right = m_pcParent->GetBounds().right - GetScrollOffset(  ).x;
	pcCol->SetFrame( cFrame );

}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::MouseDown( const Point & cPosition, uint32 nButton )
{
	if( m_pcMainView->m_cColMap.empty() )
	{
		return;
	}
	if( cPosition.y >= m_pcMainView->GetFrame().top )
	{
		return;
	}
	Point cMVPos = m_pcMainView->ConvertFromParent( cPosition );

	for( uint i = 0; i < m_pcMainView->m_cColMap.size(); ++i )
	{
		Rect cFrame( m_pcMainView->GetColumn( i )->GetFrame() );

		if( cMVPos.x >= cFrame.left && cMVPos.x <= cFrame.right )
		{
			if( i > 0 && cMVPos.x >= cFrame.left && cMVPos.x <= cFrame.left + 5 )
			{
				m_nSizeColumn = i - 1;
				m_cHitPos = cPosition;
				MakeFocus( true );
				break;
			}
			if( cMVPos.x >= cFrame.right - 5 && cMVPos.x <= cFrame.right )
			{
				m_nSizeColumn = i;
				m_cHitPos = cPosition;
				MakeFocus( true );
				break;
			}
			m_nDragColumn = i;
			break;
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::MouseUp( const Point & cPosition, uint32 nButton, Message * pcData )
{
	m_nSizeColumn = -1;

	if( m_nDragColumn != -1 && m_nDragColumn < int ( m_pcMainView->m_cColMap.size() ) )
	{
		m_pcMainView->m_nSortColumn = m_pcMainView->m_cColMap[m_nDragColumn];

		if( ( m_pcMainView->m_nModeFlags & ListView::F_NO_SORT ) == 0 )
		{
			m_pcParent->Sort();
			Rect cBounds = m_pcMainView->GetBounds();

			for( uint j = 0; j < m_pcMainView->m_cColMap.size(); ++j )
			{
				if( m_pcMainView->GetColumn( j ) != NULL )
				{
					m_pcMainView->GetColumn( j )->Invalidate( cBounds );
				}
			}
			Flush();
		}
	}

	m_nDragColumn = -1;
	if( pcData != NULL )
	{
		int nColumn;

		if( pcData->FindInt( "column", &nColumn ) == 0 )
		{
			Point cMVPos = m_pcMainView->ConvertFromParent( cPosition );

			for( uint i = 0; i < m_pcMainView->m_cColMap.size(); ++i )
			{
				Rect cFrame( m_pcMainView->GetColumn( i )->GetFrame() );

				if( cMVPos.x >= cFrame.left && cMVPos.x <= cFrame.right )
				{
					int j;

					if( cMVPos.x < cFrame.left + cFrame.Width() * 0.5f )
					{
						j = i;
					}
					else
					{
						j = i + 1;
					}
					m_pcMainView->m_cColMap.insert( m_pcMainView->m_cColMap.begin() + j, nColumn );
					for( int k = 0; k < int ( m_pcMainView->m_cColMap.size() ); ++k )
					{
						if( k != j && m_pcMainView->m_cColMap[k] == nColumn )
						{
							m_pcMainView->m_cColMap.erase( m_pcMainView->m_cColMap.begin() + k );
							break;
						}
					}
					m_pcMainView->LayoutColumns();
					Paint( GetBounds() );
					Flush();
				}
			}
		}
		else
		{
			View::MouseUp( cPosition, nButton, pcData );
		}
		return;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::FrameSized( const Point & cDelta )
{
	Rect cBounds( GetBounds() );
	bool bNeedFlush = false;

	if( cDelta.x != 0.0f )
	{
		Rect cDamage = cBounds;

		cDamage.left = cDamage.right - std::max( 2.0f, cDelta.x + 1.0f );
		Invalidate( cDamage );
		bNeedFlush = true;
	}
	if( cDelta.y != 0.0f )
	{
		Rect cDamage = cBounds;

		cDamage.top = cDamage.bottom - std::max( 2.0f, cDelta.y + 1.0f );
		Invalidate( cDamage );
		bNeedFlush = true;
	}

//      Layout();

	if( bNeedFlush )
	{
		Flush();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListViewHeader::Layout()
{
	font_height sHeight;

	GetFontHeight( &sHeight );

	if( m_pcMainView->m_nModeFlags & ListView::F_NO_HEADER )
	{
		m_vHeaderHeight = 0.0f;
	}
	else
	{
		m_vHeaderHeight = sHeight.ascender + sHeight.descender + 6.0f;
	}

	Rect cFrame = GetFrame().Bounds(  );

	cFrame.Floor();
	cFrame.top += m_vHeaderHeight;
	cFrame.right = COORD_MAX;
	m_pcMainView->SetFrame( cFrame );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListView::ListView( const Rect & cFrame, const String& cTitle, uint32 nModeFlags, uint32 nResizeMask, uint32 nFlags ):Control( cFrame, cTitle, "", NULL, nResizeMask, nFlags )
{
	m = new Private;

	m->m_pcHeaderView = new ListViewHeader( this, Rect( 0, 0, 1, 1 ), nModeFlags );
	m->m_pcMainView = m->m_pcHeaderView->m_pcMainView;
	m->m_nModeFlags = nModeFlags;

	AddChild( m->m_pcHeaderView );
	Layout();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

ListView::~ListView()
{
	delete m;
}

void ListView::LabelChanged( const String & cNewLabel )
{
}

void ListView::EnableStatusChanged( bool bIsEnabled )
{
}

bool ListView::Invoked( Message * pcMessage )
{
	Control::Invoked( pcMessage );
	return ( true );
}

bool ListView::IsMultiSelect() const
{
	return ( m->m_pcMainView->m_nModeFlags & F_MULTI_SELECT );
}

void ListView::SetMultiSelect( bool bMulti )
{
	if( bMulti )
	{
		m->m_pcMainView->m_nModeFlags |= F_MULTI_SELECT;
	}
	else
	{
		m->m_pcMainView->m_nModeFlags &= ~F_MULTI_SELECT;
	}
}

bool ListView::IsAutoSort() const
{
	return ( ( m->m_pcMainView->m_nModeFlags & F_NO_AUTO_SORT ) == 0 );
}

void ListView::SetAutoSort( bool bAuto )
{
	if( bAuto )
	{
		m->m_pcMainView->m_nModeFlags &= ~F_NO_AUTO_SORT;
	}
	else
	{
		m->m_pcMainView->m_nModeFlags |= F_NO_AUTO_SORT;
	}
}

bool ListView::HasBorder() const
{
	return ( m->m_pcMainView->m_nModeFlags & F_RENDER_BORDER );
}

void ListView::SetRenderBorder( bool bRender )
{
	if( bRender )
	{
		m->m_pcMainView->m_nModeFlags |= F_RENDER_BORDER;
	}
	else
	{
		m->m_pcMainView->m_nModeFlags &= ~F_RENDER_BORDER;
	}
	Layout();
	Flush();
}

bool ListView::HasColumnHeader() const
{
	return ( ( m->m_pcMainView->m_nModeFlags & F_NO_HEADER ) == 0 );
}

/** Turn column header on or off
 *	\par	Description:
 *			Use this method to specify if a column header should be drawn.
 *			
 * \sa HasColumnHeader()
 * \author Kurt Skauen
 *****************************************************************************/
void ListView::SetHasColumnHeader( bool bFlag )
{
	if( bFlag )
	{
		m->m_pcMainView->m_nModeFlags &= ~F_NO_HEADER;
	}
	else
	{
		m->m_pcMainView->m_nModeFlags |= F_NO_HEADER;
	}

	Layout();
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool ListView::HasFocus( void ) const
{
	if( View::HasFocus() )
	{
		return ( true );
	}
	return ( m->m_pcHeaderView->HasFocus() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::AllAttached()
{
	View::AllAttached();
	SetTarget( GetWindow() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	if( m->m_pcMainView->HandleKey( pzString[0], nQualifiers ) == false )
	{
		View::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::SetSelChangeMsg( Message * pcMsg )
{
	if( pcMsg != m->m_pcSelChangeMsg )
	{
		delete m->m_pcSelChangeMsg;

		m->m_pcSelChangeMsg = pcMsg;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::SetInvokeMsg( Message * pcMsg )
{
	if( pcMsg != m->m_pcInvokeMsg )
	{
		delete m->m_pcInvokeMsg;

		m->m_pcInvokeMsg = pcMsg;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Message *ListView::GetSelChangeMsg() const
{
	return ( m->m_pcSelChangeMsg );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Message *ListView::GetInvokeMsg() const
{
	return ( m->m_pcInvokeMsg );
}


/** Adjust scroll bars
 *	\par	Description:
 *			Calculate and set the range and proportion of scroll bars.
 *			Scroll the view so upper/left corner
 *			is at or above/left of 0,0 if needed
 *			
 * \author Kurt Skauen
 *****************************************************************************/
void ListView::AdjustScrollBars( bool bOkToHScroll )
{
	if( m->m_pcMainView->m_cRows.size() == 0 )
	{
		if( m->m_pcVScroll != NULL )
		{
			RemoveChild( m->m_pcVScroll );
			delete m->m_pcVScroll;
		}
		if( m->m_pcHScroll != NULL )
		{
			RemoveChild( m->m_pcHScroll );
			delete m->m_pcHScroll;
		}
		if( m->m_pcVScroll != NULL || m->m_pcHScroll != NULL )
		{
			m->m_pcVScroll = NULL;
			m->m_pcHScroll = NULL;
			m->m_pcMainView->ScrollTo( 0, 0 );
			m->m_pcHeaderView->ScrollTo( 0, 0 );
			Layout();
		}
	}
	else
	{
		float vViewHeight = m->m_pcMainView->GetBounds().Height(  ) + 1.0f;
		float vViewWidth = m->m_pcHeaderView->GetBounds().Width(  ) + 1.0f;
		float vContentHeight = m->m_pcMainView->m_vContentHeight;

		float vProportion;

		if( vContentHeight > 0 && vContentHeight > vViewHeight )
		{
			vProportion = vViewHeight / vContentHeight;
		}
		else
		{
			vProportion = 1.0f;
		}
		if( vContentHeight > vViewHeight )
		{
			if( m->m_pcVScroll == NULL )
			{
				m->m_pcVScroll = new ScrollBar( Rect( -1, -1, 0, 0 ), "v_scroll", NULL, 0, 1000, VERTICAL );
				AddChild( m->m_pcVScroll );
				m->m_pcVScroll->SetScrollTarget( m->m_pcMainView );
				Layout();
			}
			else
			{
				m->m_pcVScroll->SetSteps( ceil( vContentHeight / float ( m->m_pcMainView->m_cRows.size() ) ), ceil( vViewHeight * 0.8f ) );

				m->m_pcVScroll->SetProportion( vProportion );
				m->m_pcVScroll->SetMinMax( 0, std::max( vContentHeight - vViewHeight, 0.0f ) );
			}
		}
		else
		{
			if( m->m_pcVScroll != NULL )
			{
				RemoveChild( m->m_pcVScroll );
				delete m->m_pcVScroll;

				m->m_pcVScroll = NULL;
				m->m_pcMainView->ScrollTo( 0, 0 );
				Layout();
			}
		}

		if( m->m_pcVScroll != NULL )
		{
			vViewWidth -= m->m_pcVScroll->GetFrame().Width(  );
		}

		if( m->m_pcMainView->m_vTotalWidth > 0 && m->m_pcMainView->m_vTotalWidth > vViewWidth )
		{
			vProportion = vViewWidth / m->m_pcMainView->m_vTotalWidth;
		}
		else
		{
			vProportion = 1.0f;
		}

		if( m->m_pcMainView->m_vTotalWidth > vViewWidth )
		{
			if( m->m_pcHScroll == NULL )
			{
				m->m_pcHScroll = new ScrollBar( Rect( -1, -1, 0, 0 ), "h_scroll", NULL, 0, 1000, HORIZONTAL );
				AddChild( m->m_pcHScroll );
				m->m_pcHScroll->SetScrollTarget( m->m_pcHeaderView );
				Layout();
			}
			else
			{
				m->m_pcHScroll->SetSteps( 15.0f, ceil( vViewWidth * 0.8f ) );
				m->m_pcHScroll->SetProportion( vProportion );
				m->m_pcHScroll->SetMinMax( 0, m->m_pcMainView->m_vTotalWidth - vViewWidth );
			}
		}
		else
		{
			if( m->m_pcHScroll != NULL )
			{
				RemoveChild( m->m_pcHScroll );
				delete m->m_pcHScroll;

				m->m_pcHScroll = NULL;
				m->m_pcHeaderView->ScrollTo( 0, 0 );
				Layout();
			}
		}
		if( bOkToHScroll )
		{
			float nOff = m->m_pcHeaderView->GetScrollOffset().x;

			if( nOff < 0 )
			{
				vViewWidth = m->m_pcHeaderView->GetBounds().Width(  ) + 1.0f;
				if( vViewWidth - nOff > m->m_pcMainView->m_vTotalWidth )
				{
					float nDeltaScroll = std::min( ( vViewWidth - nOff ) - m->m_pcMainView->m_vTotalWidth, -nOff );

					m->m_pcHeaderView->ScrollBy( nDeltaScroll, 0 );
				}
				else if( vViewWidth > m->m_pcMainView->m_vTotalWidth )
				{
					m->m_pcHeaderView->ScrollBy( -nOff, 0 );
				}
			}
		}
		float nOff = m->m_pcMainView->GetScrollOffset().y;

		if( nOff < 0 )
		{
			vViewHeight = m->m_pcMainView->GetBounds().Height(  ) + 1.0f;
			if( vViewHeight - nOff > vContentHeight )
			{
				float nDeltaScroll = std::min( ( vViewHeight - nOff ) - vContentHeight, -nOff );

				m->m_pcMainView->ScrollBy( 0, nDeltaScroll );
			}
			else if( vViewHeight > vContentHeight )
			{
				m->m_pcMainView->ScrollBy( 0, -nOff );
			}
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::Layout()
{
	font_height sHeight;

	GetFontHeight( &sHeight );

	float nTopHeight = m->m_pcHeaderView->m_vHeaderHeight;

	Rect cFrame( GetBounds().Bounds(  ) );

	cFrame.Floor();
	if( m->m_pcMainView->m_nModeFlags & F_RENDER_BORDER )
	{
		cFrame.Resize( 2.0f, 2.0f, -2.0f, -2.0f );
	}

	Rect cHeaderFrame( cFrame );

	if( m->m_pcHScroll != NULL )
	{
		cHeaderFrame.bottom -= 16.0f;
	}
	if( m->m_pcVScroll != NULL )
	{
		Rect cVScrFrame( cFrame );

		cVScrFrame.top += nTopHeight;
		//      cVScrFrame.bottom -= 2.0f;
		if( m->m_pcHScroll != NULL )
		{
			cVScrFrame.bottom -= 16.0f;
		}
		cVScrFrame.left = cVScrFrame.right - 16.0f;

		m->m_pcVScroll->SetFrame( cVScrFrame );
	}
	if( m->m_pcHScroll != NULL )
	{
		Rect cVScrFrame( cFrame );

		cHeaderFrame.bottom = floor( cHeaderFrame.bottom );
		if( m->m_pcVScroll != NULL )
		{
			cVScrFrame.right -= 16.0f;
		}
		//      cVScrFrame.left  += 2.0f;
		//      cVScrFrame.right -= 2.0f;
		cVScrFrame.top = cVScrFrame.bottom - 16.0f;
		cHeaderFrame.bottom = cVScrFrame.top;

		m->m_pcHScroll->SetFrame( cVScrFrame );
	}

	m->m_pcHeaderView->SetFrame( cHeaderFrame );
	m->m_pcHeaderView->Layout();
	AdjustScrollBars();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::FrameSized( const Point & cDelta )
{
	if( m->m_pcMainView->m_cColMap.empty() == false )
	{
		ListViewCol *pcCol = m->m_pcMainView->m_cCols[m->m_pcMainView->m_cColMap[m->m_pcMainView->m_cColMap.size() - 1]];
		Rect cFrame = pcCol->GetFrame();

		cFrame.right = GetBounds().right - m->m_pcHeaderView->GetScrollOffset(  ).x;

		pcCol->SetFrame( cFrame );
	}

	Layout();
	Flush();
}

/** Sort rows
 *	\par	Description:
 *			Sort rows in the ListView by the selected column, according to
 *			ListViewRow::IsLessThan().
 *			The actual sorting takes place in SortRows(), which can be overridden
 *			to provide custom sorting routines.
 * \sa SortRows()
 * \author Kurt Skauen, Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void ListView::Sort()
{
	if( uint ( m->m_pcMainView->m_nSortColumn ) >= m->m_pcMainView->m_cCols.size() )
	{
		return;
	}
	if( m->m_pcMainView->m_cRows.empty() )
	{
		return;
	}

	SortRows( &m->m_pcMainView->m_cRows, m->m_pcMainView->m_nSortColumn );

	if( m->m_pcMainView->m_nFirstSel != -1 )
	{
		for( uint i = 0; i < m->m_pcMainView->m_cRows.size(); ++i )
		{
			if( m->m_pcMainView->m_cRows[i]->IsSelected() )
			{
				m->m_pcMainView->m_nFirstSel = i;
				break;
			}
		}
		for( int i = m->m_pcMainView->m_cRows.size() - 1; i >= 0; --i )
		{
			if( m->m_pcMainView->m_cRows[i]->IsSelected() )
			{
				m->m_pcMainView->m_nLastSel = i;
				break;
			}
		}
	}

	RefreshLayout();
}

/** Make row visible
 *	\par	Description:
 *			Scroll the ListView so that a row becomes visible.
 *	\param	nRow Index to row to make visible.
 *	\param	bCenter Set to true to have the view centred on nRow.
 * \author Kurt Skauen
 *****************************************************************************/
void ListView::MakeVisible( int nRow, bool bCenter )
{
	m->m_pcMainView->MakeVisible( nRow, bCenter );
	Flush();
}

int ListView::InsertColumn( const char *pzTitle, int nWidth, int nPos )
{
	int nColumn = m->m_pcMainView->InsertColumn( pzTitle, nWidth, nPos );

	Layout();
	Flush();
	return ( nColumn );
}

const std::vector < int >&ListView::GetColumnMapping() const
{
	return ( m->m_pcMainView->m_cColMap );
}

void ListView::SetColumnMapping( const std::vector < int >&cMap )
{
	for( int i = 0; i < int ( m->m_pcMainView->m_cCols.size() ); ++i )
	{
		if( std::find( cMap.begin(), cMap.end(  ), i ) == cMap.end(  ) )
		{
			if( std::find( m->m_pcMainView->m_cColMap.begin(), m->m_pcMainView->m_cColMap.end(  ), i ) != m->m_pcMainView->m_cColMap.end(  ) )
			{
				m->m_pcMainView->m_cCols[i]->Show( false );
			}
		}
		else
		{
			if( std::find( m->m_pcMainView->m_cColMap.begin(), m->m_pcMainView->m_cColMap.end(  ), i ) == m->m_pcMainView->m_cColMap.end(  ) )
			{
				m->m_pcMainView->m_cCols[i]->Show( true );
			}
		}
	}
	m->m_pcMainView->m_cColMap = cMap;
	m->m_pcMainView->LayoutColumns();
	m->m_pcHeaderView->Invalidate();
}

/** Insert a row
 *	\par	Description:
 *			Insert a row at a specific position.
 *	\param	nPos Row index.
 *	\param	bUpdate Update display. If many operations are performed, often
 *			only the last one need to update the display.
 * \author Kurt Skauen
 *****************************************************************************/
void ListView::InsertRow( int nPos, ListViewRow * pcRow, bool bUpdate )
{
	m->m_pcMainView->InsertRow( nPos, pcRow, bUpdate );
	AdjustScrollBars();
	if( bUpdate )
	{
		Flush();
	}
}

/** Insert a row
 *	\par	Description:
 *			Insert a row at the end of the list.
 *	\param	bUpdate Update display. If many operations are performed, often
 *			only the last one need to update the display.
 * \author Kurt Skauen
 *****************************************************************************/
void ListView::InsertRow( ListViewRow * pcRow, bool bUpdate )
{
	InsertRow( -1, pcRow, bUpdate );
}

/** Remove a row
 *	\par	Description:
 *			Removes a row from the ListView. A pointer to the removed row is
 *			returned, and the application is responsible for freeing it.
 *	\param	nIndex Row index.
 *	\param	bUpdate Update display. If many operations are performed, often
 *			only the last one need to update the display.
 * \author Kurt Skauen
 *****************************************************************************/
ListViewRow *ListView::RemoveRow( int nIndex, bool bUpdate )
{
	ListViewRow *pcRow = m->m_pcMainView->RemoveRow( nIndex, bUpdate );

	AdjustScrollBars();
	if( bUpdate )
	{
		Flush();
	}
	return ( pcRow );
}

/** Refresh row
 *	\par	Description:
 *			This method causes a row to be redrawn.
 *	\param	nIndex Row index.
 *	\param	nFlags Flags. Use INV_VISUAL.
 * \author Kurt Skauen
 *****************************************************************************/
void ListView::InvalidateRow( int nRow, uint32 nFlags )
{
	m->m_pcMainView->InvalidateRow( nRow, nFlags );
	Flush();
}

/** Get row count
 *	\par	Description:
 *			Returns the number of rows currently attached to the ListView.
 * \author Kurt Skauen
 *****************************************************************************/
uint ListView::GetRowCount() const
{
	return ( m->m_pcMainView->m_cRows.size() );
}

/** Get a row
 *	\par	Description:
 *			Returns the a pointer to the row at index nIndex or NULL if the
 *			index is out of bounds.
 *	\param	nIndex Row index.
 * \author Kurt Skauen
 *****************************************************************************/
ListViewRow *ListView::GetRow( uint nIndex ) const
{
	if( nIndex < m->m_pcMainView->m_cRows.size() )
	{
		return ( m->m_pcMainView->m_cRows[nIndex] );
	}
	else
	{
		return ( NULL );
	}
}

/** Find row at a certain position
 *	\par	Description:
 *			Returns a pointer to the row that is located at cPos, or NULL
 *			if no row was found at that position. Can for example be used to
 *			implement pop-up menus.
 *	\param	cPos Coordinates referenced to ListView control.
 * \author Kurt Skauen
 *****************************************************************************/
ListViewRow *ListView::GetRow( const Point & cPos ) const
{
	int nHitRow = m->m_pcMainView->GetRowIndex( cPos.y );

	if( nHitRow >= 0 )
	{
		return ( m->m_pcMainView->m_cRows[nHitRow] );
	}
	else
	{
		return ( NULL );
	}
}

/** Find row at a certain position
 *	\par	Description:
 *			Returns the row index to the row that is located at cPos, or -1
 *			if no row was found at that position. Can for example be used to
 *			implement pop-up menus.
 *	\param	cPos Screen coordinate to look for a row at.
 * \author Kurt Skauen
 *****************************************************************************/
int ListView::HitTest( const Point & cPos ) const
{
	Point cParentPos = ConvertToScreen( m->m_pcMainView->ConvertFromScreen( cPos ) );
	int nHitRow = m->m_pcMainView->GetRowIndex( cParentPos.y );

	if( nHitRow >= 0 )
	{
		return ( nHitRow );
	}
	else
	{
		return ( -1 );
	}
}

/** Get row vertical position
 *	\par	Description:
 *			Returns the vertical position, in reference to the ListView control,
 *			of a row with index nRow, or 0.0 if no row was found at that
 *			position.
 *	\param	nRow Row index to look at.
 * \author Kurt Skauen
 *****************************************************************************/
float ListView::GetRowPos( int nRow )
{
	if( nRow >= 0 && nRow < int ( m->m_pcMainView->m_cRows.size() ) )
	{
		return ( m->m_pcMainView->m_cRows[nRow]->m_vYPos + m->m_pcMainView->GetFrame().top + m->m_pcMainView->GetScrollOffset(  ).y );
	}
	else
	{
		return ( 0.0f );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::Select( int nFirst, int nLast, bool bReplace, bool bSelect )
{
	if( m->m_pcMainView->m_cRows.empty() )
	{
		return;
	}
	if( ( bReplace || ( m->m_pcMainView->m_nModeFlags & F_MULTI_SELECT ) == 0 ) && m->m_pcMainView->m_nFirstSel != -1 )
	{
		for( int i = m->m_pcMainView->m_nFirstSel; i <= m->m_pcMainView->m_nLastSel; ++i )
		{
			if( m->m_pcMainView->m_cRows[i]->IsSelected() )
			{
				m->m_pcMainView->m_cRows[i]->SetSelected( false );
				m->m_pcMainView->InvalidateRow( i, INV_VISUAL );
			}
		}
		m->m_pcMainView->m_nFirstSel = -1;
		m->m_pcMainView->m_nLastSel = -1;
	}
	if( m->m_pcMainView->SelectRange( nFirst, nLast, bSelect ) )
	{
		SelectionChanged( m->m_pcMainView->m_nFirstSel, m->m_pcMainView->m_nLastSel );
	}
	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::Select( int nRow, bool bReplace, bool bSelect )
{
	Select( nRow, nRow, bReplace, bSelect );
}

void ListView::ClearSelection()
{
	if( m->m_pcMainView->ClearSelection() )
	{
		SelectionChanged( -1, -1 );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::Highlight( int nFirst, int nLast, bool bReplace, bool bHighlight )
{
	if( nLast < 0 )
	{
		return;
	}
	if( nFirst < 0 )
	{
		nFirst = 0;
	}
	if( nFirst >= int ( m->m_pcMainView->m_cRows.size() ) )
	{
		return;
	}
	if( nLast >= int ( m->m_pcMainView->m_cRows.size() ) )
	{
		nLast = m->m_pcMainView->m_cRows.size() - 1;
	}

	if( bReplace )
	{
		for( int i = 0; i < int ( m->m_pcMainView->m_cRows.size() ); ++i )
		{
			bool bHigh = ( i >= nFirst && i <= nLast );

			if( m->m_pcMainView->m_cRows[i]->IsHighlighted() != bHigh )
			{
				m->m_pcMainView->m_cRows[i]->SetHighlighted( bHigh );
				m->m_pcMainView->InvalidateRow( i, ListView::INV_VISUAL );
			}
		}
	}
	else
	{
		for( int i = nFirst; i <= nLast; ++i )
		{
			if( m->m_pcMainView->m_cRows[i]->IsHighlighted() != bHighlight )
			{
				m->m_pcMainView->m_cRows[i]->SetHighlighted( bHighlight );
				m->m_pcMainView->InvalidateRow( i, ListView::INV_VISUAL );
			}
		}
	}
	Flush();
}

void ListView::SetCurrentRow( int nRow )
{
	if( m->m_pcMainView->m_cRows.empty() )
	{
		return;
	}
	if( nRow < 0 )
	{
		nRow = 0;
	}
	else if( nRow >= int ( m->m_pcMainView->m_cRows.size() ) )
	{
		nRow = m->m_pcMainView->m_cRows.size() - 1;
	}
	m->m_pcMainView->m_nEndSel = nRow;
}

void ListView::Highlight( int nRow, bool bReplace, bool bHighlight )
{
	Highlight( nRow, nRow, bReplace, bHighlight );
}

/** Check if a row is selected
 *	\par	Description:
 *			Returns true if nRow is selected.
 * \param	nRow Row to check.
 * \author Kurt Skauen
 *****************************************************************************/
bool ListView::IsSelected( uint nRow ) const
{
	if( nRow < m->m_pcMainView->m_cRows.size() )
	{
		return ( m->m_pcMainView->m_cRows[nRow]->IsSelected() );
	}
	else
	{
		return ( false );
	}
}

/** Clear list
 *	\par	Description:
 *			Clears the ListView for all rows.
 * \author Kurt Skauen
 *****************************************************************************/
void ListView::Clear()
{
	m->m_pcMainView->Clear();
	AdjustScrollBars();
	Flush();
}

void ListView::Paint( const Rect & cUpdateRect )
{
	if( m->m_pcVScroll != NULL && m->m_pcHScroll != NULL )
	{
		Rect cHFrame = m->m_pcHScroll->GetFrame();
		Rect cVFrame = m->m_pcVScroll->GetFrame();
		Rect cFrame( cHFrame.right + 1, cVFrame.bottom + 1, cVFrame.right, cHFrame.bottom );

		FillRect( cFrame, get_default_color( COL_NORMAL ) );
	}
	if( m->m_pcMainView->m_nModeFlags & F_RENDER_BORDER )
	{
		DrawFrame( Rect( GetBounds() ), FRAME_RECESSED | FRAME_TRANSPARENT );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::Invoked( int nFirstRow, int nLastRow )
{
	if( m->m_pcInvokeMsg != NULL )
	{
		Invoke( m->m_pcInvokeMsg );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::SelectionChanged( int nFirstRow, int nLastRow )
{
	if( m->m_pcSelChangeMsg != NULL )
	{
		Invoke( m->m_pcSelChangeMsg );
	}
}

/** Get first selected row
 *	\par	Description:
 *			Returns index to the first select row, or -1 if none is selected.
 * \author Kurt Skauen
 *****************************************************************************/
int ListView::GetFirstSelected() const
{
	return ( m->m_pcMainView->m_nFirstSel );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::StartScroll( scroll_direction eDirection, bool bSelect )
{
	m->m_pcMainView->StartScroll( eDirection, bSelect );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void ListView::StopScroll()
{
	m->m_pcMainView->StopScroll();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ListView::GetLastSelected() const
{
	return ( m->m_pcMainView->m_nLastSel );
}

bool ListView::DragSelection( const Point & cPos )
{
	return ( false );
}


/** STL iterator interface
 *	\par	Description:
 *			Returns iterator pointing to the first row.
 * \author Kurt Skauen
 *****************************************************************************/
ListView::const_iterator ListView::begin() const
{
	return ( m->m_pcMainView->m_cRows.begin() );
}

/** STL iterator interface
 *	\par	Description:
 *			Returns iterator pointing past the last row.
 * \author Kurt Skauen
 *****************************************************************************/
ListView::const_iterator ListView::end() const
{
	return ( m->m_pcMainView->m_cRows.end() );
}

void ListView::__LV_reserved_1__()
{
}
void ListView::__LV_reserved_2__()
{
}
void ListView::__LV_reserved_3__()
{
}
void ListView::__LV_reserved_4__()
{
}
void ListView::__LV_reserved_5__()
{
}

/** Refresh layout
 *	\par	Description:
 *			Calculates new positions for all rows, invalidates them so that
 *			they are rendered, and adjusts scrollbars. Normally there is no
 *			reason to use this function, it is primarily implemented for use
 *			by the TreeView class.
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void ListView::RefreshLayout()
{
	m->m_pcMainView->LayoutRows();

	for( uint i = 0; i < m->m_pcMainView->m_cColMap.size(); ++i )
	{
		m->m_pcMainView->GetColumn( i )->Invalidate();
		m->m_pcMainView->GetColumn( i )->Flush();
	}

	AdjustScrollBars( false );

	Flush();
	Sync();
}

/** Sort rows
 *	\par	Description:
 *			Overridable default sorting routine.
 *	\param	pcRows std::vector of ListViewRows to be sorted.
 *	\param	nColumn The column to sort by.
 * \sa Sort()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void ListView::SortRows( std::vector < ListViewRow * >*pcRows, int nColumn )
{
	std::sort( pcRows->begin(), pcRows->end(  ), RowContentPred( nColumn ) );
}

void ListView::MouseDown(const Point& cPos,uint32 nButtons)
{
	View::MouseDown(cPos,nButtons);
}

void ListView::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	View::MouseMove(cNewPos,nCode,nButtons,pcData);
}

void ListView::MouseUp( const Point & cPosition, uint32 nButton, Message * pcData )
{
	View::MouseUp(cPosition,nButton,pcData);
}

