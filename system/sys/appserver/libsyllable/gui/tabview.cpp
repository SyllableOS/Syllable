
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

#include <stdio.h>

#include <gui/tabview.h>
#include <gui/window.h>
#include <util/message.h>

inline float max( float a, float b )
{
	return ( ( a < b ) ? b : a );
}

using namespace os;

/** Contruct the TabView
 * \par Description:
 *	Not much happens here. All paramaters are sendt ot the View() constructor
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

TabView::TabView( const Rect & cFrame, const char *pzTitle, uint32 nResizeMask, uint32 nFlags ):View( cFrame, pzTitle, nResizeMask, nFlags & ~WID_FULL_UPDATE_ON_RESIZE )
{
	GetFontHeight( &m_sFontHeight );
	m_vGlyphHeight = m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap;
	m_vTabHeight = ceil( m_vGlyphHeight * 1.2f + 6.0f );
	m_nSelectedTab = 0;
	m_vScrollOffset = 0;
	m_bMouseClicked = false;
	m_vTotalWidth = 4.0f;

	m_pcTopView = new TopView( Rect( 2, 0, cFrame.Width() + 1.0f - 3.0f, m_vTabHeight - 3.0f ), this );
	AddChild( m_pcTopView );
}

void TabView::FrameSized( const Point & cDelta )
{
	Rect cBounds = GetBounds();
	bool bNeedFlush = false;

	if( m_nSelectedTab < m_cTabList.size() )
	{
		View *pcView = m_cTabList[m_nSelectedTab].m_pcView;

		if( pcView != NULL )
		{
			Rect cClientBounds = cBounds;

			cClientBounds.Resize( 2.0f, m_vTabHeight, -2.0f, -2.0f );
			cClientBounds.Floor();
			pcView->SetFrame( cClientBounds );
		}
	}
	if( cDelta.x != 0.0f )
	{
		Rect cDamage = cBounds;

		cDamage.left = cDamage.right - max( 2.0f, cDelta.x + 8.0f );
		Invalidate( cDamage );
		bNeedFlush = true;
	}
	if( cDelta.y != 0.0f )
	{
		Rect cDamage = cBounds;

		cDamage.top = cDamage.bottom - max( 2.0f, cDelta.y + 1.0f );
		Invalidate( cDamage );
		bNeedFlush = true;
	}

	cBounds.Floor();
	float vWidth = cBounds.Width() + 1.0f;

	float nOldOffset = m_vScrollOffset;

	if( vWidth < m_vTotalWidth )
	{
		m_pcTopView->SetFrame( Rect( cBounds.left + 8.0f, cBounds.top, cBounds.right - 8.0f, m_vTabHeight - 3.0f ) );
		vWidth -= 16.0f;
		if( m_vScrollOffset > 0 )
		{
			m_vScrollOffset = 0;
		}
		if( m_vTotalWidth + m_vScrollOffset < vWidth )
		{
			m_vScrollOffset = vWidth - m_vTotalWidth;
		}
		if( vWidth + 16.0f - cDelta.x >= m_vTotalWidth )
		{
			m_pcTopView->Invalidate();
			Rect cDamage( cBounds );

			cDamage.bottom = cDamage.top + m_vTabHeight;
			Invalidate( cDamage );
			bNeedFlush = true;
		}
	}
	else
	{
		m_pcTopView->SetFrame( Rect( cBounds.left + 2, cBounds.top, cBounds.right - 2, m_vTabHeight - 3 ) );
		if( m_vScrollOffset > vWidth - m_vTotalWidth )
		{
			m_vScrollOffset = vWidth - m_vTotalWidth;
		}
		if( m_vScrollOffset < 0 )
		{
			m_vScrollOffset = 0;
		}
		if( vWidth - cDelta.x < m_vTotalWidth )
		{
			m_pcTopView->Invalidate();
			Rect cDamage( cBounds );

			cDamage.bottom = cDamage.top + m_vTabHeight;
			Invalidate( cDamage );
			bNeedFlush = true;
		}
	}
	if( m_vScrollOffset != nOldOffset )
	{
		m_pcTopView->ScrollBy( m_vScrollOffset - nOldOffset, 0 );
		m_pcTopView->Invalidate();
		Rect cDamage( cBounds );

		cDamage.bottom = cDamage.top + m_vTabHeight;
		Invalidate( cDamage );
		bNeedFlush = true;
	}
	if( bNeedFlush )
	{
		Flush();
	}
}

/** 
 * \par Description:
 *	TabView overloades AllAttached to set the window as it's message target.
 * \sa Invoker::SetTarget(), Invoker::SetMessage(), View::AllAttached()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


void TabView::AllAttached()
{
	SetTarget( GetWindow() );
	View::AllAttached();
}

/** Add a tab at the end of the list.
 * \par Description:
 *	Add a new tab to the TabView. The new tab will be appended to the list
 *	and will appear rightmost. Each tab is associated with title that is
 *	printed inside it, and a View that will be made visible when the tab
 *	is selected or hidden when the tab is inactive. If you want to use
 *	the TabView to something else than flipping between views, you can
 *	pass a NULL pointer, or you can pass the same ViewPointer for all tabs.
 *	Then you can associate a message with the TabView that will be sendt
 *	to it's target everytime the selection change.
 * \par Note:
 *	Views associated with tabs should *NOT* be attached to any other
 *	view when or after the tab is created. It will automatically be a
 *	child of the TabView when added to a tab.
 *
 *	Views associated with tabs *MUST* have the CF_FOLLOW_ALL flag set to
 *	be properly resized when the TabView is resized.
 *
 *	When a view is associated with a tab, the view will be resized to
 *	fit the interiour of the TabView.
 *
 * \param pcTitle - The string that should appear inside the tab.
 * \param pcView  - The View to be accosiated with the tab, or NULL.
 *
 * \return The zero based index of the new tab.
 * \sa InsertTab(), DeleteTab(), Invoker::SetMessage(), Invoker::SetTarget()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int TabView::AppendTab( const char *pzTitle, View * pcView )
{
	return ( InsertTab( m_cTabList.size(), pzTitle, pcView ) );
}

/** Insert tabs at a given position
 * \par Description:
 *	Same as AppendTab() except that InsertTab() accept a zero based index
 *	at which the tab will be inserted. Look at AppendTab() to get the full
 *	thruth.
 * \par Note:
 *	If the rightmost tab is selected, the selection will be moved to
 *	the previous tab, and a notification message (if any) is sendt.
 * \par Warning:
 *	The index *MUST* be between 0 and the current number of tabs!
 *
 * \param nIndex  - The zero based position where the tab is inserted.
 * \param pcTitle - The string that should appear inside the tab.
 * \param pcView  - The View to be accosiated with the tab, or NULL.
 *
 * \return The zero based index of the new tab (Same as nIndex).
 * \sa AppendTab(), DeleteTab(), Invoker::SetMessage(), Invoker::SetTarget()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int TabView::InsertTab( uint nIndex, const char *pzTitle, View * pcView )
{
	m_cTabList.insert( m_cTabList.begin() + nIndex, Tab( pzTitle, pcView ) );
	if( pcView != NULL && pcView->GetParent() != this )
	{
		AddChild( pcView );
		Rect cBounds = GetNormalizedBounds();

		cBounds.Resize( 2, m_vTabHeight, -2, -2 );
		cBounds.Floor();
		pcView->SetFrame( cBounds );
		if( nIndex != m_nSelectedTab )
		{
			pcView->Show( false );
		}
	}
	m_cTabList[nIndex].m_vWidth = ceil( GetStringWidth( pzTitle ) * 1.1f ) + 4.0f;
	m_vTotalWidth += m_cTabList[nIndex].m_vWidth;
	m_pcTopView->Invalidate( GetBounds() );
	m_pcTopView->Flush();
	return ( nIndex );
}

/** Delete a given tab
 * \par Description:
 *	The tab at position nIndex is deleted, and the associated view is
 *	removed from the child list unless it is still associated with any
 *	of the remaining tabs. The view associated with the deleted tab
 *	is returned, and must be deleted by the caller if it is no longer
 *	needed.
 * \param nIndex - The zero based index of the tab to delete.
 * \return Pointer to the view associated with the deleted tab.
 * \sa AppendTab(), InsertTab(), GetTabView(), GetTabTitle()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


View *TabView::DeleteTab( uint nIndex )
{
	if( nIndex >= m_cTabList.size() )
	{
		return ( NULL );
	}

	uint nOldSelection = m_nSelectedTab;
	View *pcView = m_cTabList[nIndex].m_pcView;
	View *pcPrevSelection = NULL;

	if( m_nSelectedTab < m_cTabList.size() && m_cTabList[m_nSelectedTab].m_pcView != NULL )
	{
		pcPrevSelection = m_cTabList[m_nSelectedTab].m_pcView;
	}

	if( nIndex < m_nSelectedTab || m_nSelectedTab == ( m_cTabList.size() - 1 ) )
	{
		m_nSelectedTab--;
	}
	m_vTotalWidth -= m_cTabList[nIndex].m_vWidth;

	m_cTabList.erase( m_cTabList.begin() + nIndex );

	bool bStillMember = false;

	for( uint i = 0; i < m_cTabList.size(); ++i )
	{
		if( m_cTabList[i].m_pcView == pcView )
		{
			bStillMember = true;
			break;
		}
	}
	if( bStillMember == false )
	{
		RemoveChild( pcView );
		if( nIndex != nOldSelection )
		{
			pcView->Show();	// Leave the view as we found it.
		}
	}
	if( m_cTabList.size() > 0 )
	{
		if( pcPrevSelection != m_cTabList[m_nSelectedTab].m_pcView )
		{
			if( pcPrevSelection != NULL && pcPrevSelection != pcView )
			{
				pcPrevSelection->Hide();
			}
			if( m_cTabList[m_nSelectedTab].m_pcView != NULL )
			{
				View *pcNewView = m_cTabList[m_nSelectedTab].m_pcView;
				Rect cClientBounds = GetNormalizedBounds();

				cClientBounds.Resize( 2.0f, m_vTabHeight, -2.0f, -2.0f );
				cClientBounds.Floor();
				pcNewView->SetFrame( cClientBounds );
				pcNewView->Show( true );
				pcNewView->MakeFocus( true );
			}
		}
	}
	m_pcTopView->Invalidate();
	Invalidate();
	m_pcTopView->Flush();
	return ( pcView );
}

/** Get the View associated with a given tab.
 * \param nIndex - The zero based index of the tab.
 * \return Pointer to the View associated with the tab.
 * \sa GetTabTitle(), AppendTab(), InsertTab()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


View *TabView::GetTabView( uint nIndex ) const
{
	return ( m_cTabList[nIndex].m_pcView );
}

/** Get the title of a given tab.
 * \param nIndex - The zero based index of the tab.
 * \return const reference to a STL string containing the title.
 * \sa GetTabView(), AppendTab(), InsertTab()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

const std::string & TabView::GetTabTitle( uint nIndex ) const
{
	return ( m_cTabList[nIndex].m_cTitle );
}


int TabView::SetTabTitle( uint nIndex, const std::string & cTitle )
{
	m_cTabList[nIndex].m_cTitle = cTitle;

	float vOldWidth = m_cTabList[nIndex].m_vWidth;

	m_cTabList[nIndex].m_vWidth = ceil( GetStringWidth( cTitle ) * 1.1f ) + 4.0f;
	m_vTotalWidth += m_cTabList[nIndex].m_vWidth - vOldWidth;

	Invalidate( GetBounds() );
	m_pcTopView->Invalidate( GetBounds() );
	m_pcTopView->Flush();

	return ( 0 );
}

/** Get number of tabs currently added to the view.
 * \return Tab count
 * \sa AppendTab(), InsertTab(), DeleteTab()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int TabView::GetTabCount() const
{
	return ( m_cTabList.size() );
}

/** Get the current selection
 * \par Description:
 *	Returns the zero based index of the selected tab.
 * \par Note:
 *	This index is also added to the notification sendt when the selection
 *	change. The selection is then added under the name "selection".
 * \return The zero based index of the selected tab.
 * \sa SetSelection(), Invoker::SetMessage(), Invoker::SetTarget()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

uint TabView::GetSelection()
{
	return ( m_nSelectedTab );
}

/** Select a tab, and optionally notify the target.
 * \par Description:
 *	Selects the given tab. If bNotify is true, and a messages has been
 *	assigned through Invoker::SetMessage() the message will be sendt
 *	to the target.
 * \param nIndex - The zero based index of the tab to select.
 * \param bNotify - Set to true if Invoker::Invoke() should be called.
 * \sa GetSelection(), Invoker::Invoke(), Invoker::SetMessage(), Invoker::SetTarget()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void TabView::SetSelection( uint nIndex, bool bNotify )
{
	if( nIndex != m_nSelectedTab && nIndex < m_cTabList.size() )
	{
		if( m_nSelectedTab < m_cTabList.size() && m_cTabList[m_nSelectedTab].m_pcView != NULL )
		{
			m_cTabList[m_nSelectedTab].m_pcView->Show( false );
		}
		m_nSelectedTab = nIndex;
		if( m_cTabList[m_nSelectedTab].m_pcView != NULL )
		{
			View *pcView = m_cTabList[m_nSelectedTab].m_pcView;
			Rect cClientBounds = GetNormalizedBounds();

			cClientBounds.Resize( 2.0f, m_vTabHeight, -2.0f, -2.0f );
			cClientBounds.Floor();
			pcView->SetFrame( cClientBounds );
			pcView->Show( true );
			pcView->MakeFocus( true );
		}
		Invalidate( GetBounds() );
		m_pcTopView->Invalidate( m_pcTopView->GetBounds() );
		m_pcTopView->Flush();
		if( bNotify )
		{
			Message *pcMsg = GetMessage();

			if( pcMsg != NULL )
			{
				Message cMsg( *pcMsg );

				cMsg.AddInt32( "selection", nIndex );
				Invoke( &cMsg );
			}
		}
	}
}

void TabView::MouseDown( const Point & cPosition, uint32 nButtons )
{
	if( m_cTabList.size() == 0 )
	{
		View::MouseDown( cPosition, nButtons );
		return;
	}
	if( cPosition.y >= m_vTabHeight )
	{
		return;
	}
	m_bMouseClicked = true;
	m_cHitPos = cPosition;
	float x = m_vScrollOffset;

	for( uint i = 0; i < m_cTabList.size(); ++i )
	{
		float vWidth = m_cTabList[i].m_vWidth;

		if( cPosition.x > x && cPosition.x < x + vWidth - 1 )
		{
			SetSelection( i );
			break;
		}
		x += vWidth;
	}
}

void TabView::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	m_bMouseClicked = false;
}

void TabView::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	if( ( nButtons & 0x01 ) && m_bMouseClicked )
	{
		float nOldOffset = m_vScrollOffset;

		float vWidth = Width() + 1.0f;

		if( m_vTotalWidth <= vWidth && ( GetQualifiers() & QUAL_SHIFT ) == 0 )
		{
			return;
		}

		m_vScrollOffset += cNewPos.x - m_cHitPos.x;

		if( m_vTotalWidth <= vWidth )
		{
			if( m_vScrollOffset > vWidth - m_vTotalWidth )
			{
				m_vScrollOffset = vWidth - m_vTotalWidth;
			}
			if( m_vScrollOffset < 0 )
			{
				m_vScrollOffset = 0;
			}
		}
		else
		{
			vWidth -= 16;
			if( m_vScrollOffset > 0 )
			{
				m_vScrollOffset = 0;
			}
			if( m_vTotalWidth + m_vScrollOffset < vWidth )
			{
				m_vScrollOffset = vWidth - m_vTotalWidth;
			}
		}
		if( m_vScrollOffset != nOldOffset )
		{
			m_cHitPos = cNewPos;
			if( m_vTotalWidth <= vWidth )
			{
				Invalidate( GetBounds() );
			}
			else
			{
				Rect cBounds = GetBounds();

				Invalidate( Rect( cBounds.left, m_vTabHeight - 2, cBounds.right, m_vTabHeight ) );
			}
			m_pcTopView->ScrollBy( m_vScrollOffset - nOldOffset, 0 );
			Flush();
		}
	}
}


void TabView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	switch ( pzString[0] )
	{
	case VK_LEFT_ARROW:
		if( m_nSelectedTab > 0 )
		{
			SetSelection( m_nSelectedTab - 1 );
		}
		break;
	case VK_RIGHT_ARROW:
		if( m_nSelectedTab < m_cTabList.size() - 1 )
		{
			SetSelection( m_nSelectedTab + 1 );
		}
		break;
	default:
		View::KeyDown( pzString, pzRawString, nQualifiers );
		break;
	}
}

/** 
 * \par Description:
 *	TabView overloads GetPreferredSize() and return the largest size returned
 *	by any of the views associated with the different tabs.
 * \return The largest preffered size returned by it's childs.
 * \sa View::GetPreferredSize()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Point TabView::GetPreferredSize( bool bLargest ) const
{
	Point cSize( m_vTotalWidth, 0.0f );

	for( uint i = 0; i < m_cTabList.size(); ++i )
	{
		if( m_cTabList[i].m_pcView != NULL )
		{
			Point cTmp = m_cTabList[i].m_pcView->GetPreferredSize( bLargest );

			if( cTmp.x > cSize.x )
			{
				cSize.x = cTmp.x;
			}
			if( cTmp.y > cSize.y )
			{
				cSize.y = cTmp.y;
			}
		}
	}
	cSize.y += m_vTabHeight + 3.0f;
	cSize.x += 5.0f;
	return ( cSize );
}

static Color32_s Tint( const Color32_s & sColor, float vTint )
{
	return ( Color32_s( int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) )*0.5f ), int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) )*0.5f ), int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) )*0.5f ), sColor.alpha ) );
}


void TabView::TopView::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();

	cBounds.Floor();

	Color32_s sFgCol = get_default_color( COL_SHINE );
	Color32_s sBgCol = get_default_color( COL_SHADOW );
	Color32_s sFgShadowCol = Tint( sFgCol, 0.9f );
	Color32_s sBgShadowCol = Tint( sBgCol, 0.8f );

	float x;

	if( m_pcParent->m_vTotalWidth > m_pcParent->Width() + 1.0f )
	{
		x = 0;
	}
	else
	{
		x = -2;
	}

	float vTabHeight = m_pcParent->m_vTabHeight;
	font_height sFontHeight = m_pcParent->m_sFontHeight;
	float vGlyphHeight = m_pcParent->m_vGlyphHeight;

	FillRect( cBounds, get_default_color( COL_NORMAL ) );
	for( uint i = 0; i < m_pcParent->m_cTabList.size(); ++i )
	{
		float vWidth = m_pcParent->m_cTabList[i].m_vWidth;
		Rect cTabFrame( x + 2, 2, x + vWidth - 1 + 2, m_pcParent->m_vTabHeight - 1 );

		cTabFrame.Floor();
		if( i == m_pcParent->m_nSelectedTab )
		{
			cTabFrame.Resize( -2, -2, 2, 0 );
		}

		SetFgColor( sFgShadowCol );
		if( i != m_pcParent->m_nSelectedTab + 1 )
		{
			DrawLine( Point( cTabFrame.left, cTabFrame.bottom - 2 ), Point( cTabFrame.left, cTabFrame.top + 3 ) );
			DrawLine( Point( cTabFrame.left + 3, cTabFrame.top ) );
			DrawLine( Point( cTabFrame.right, cTabFrame.top ) );

			SetFgColor( sFgCol );

			DrawLine( Point( cTabFrame.left + 1, cTabFrame.bottom - 1 ), Point( cTabFrame.left + 1, cTabFrame.top + 4 ) );
			DrawLine( Point( cTabFrame.left + 4, cTabFrame.top + 1 ) );
			DrawLine( Point( cTabFrame.right - 1, cTabFrame.top + 1 ) );
		}
		else
		{
			DrawLine( Point( cTabFrame.left + 2, cTabFrame.top ), Point( cTabFrame.right, cTabFrame.top ) );
			SetFgColor( sFgCol );
			DrawLine( Point( cTabFrame.left + 2, cTabFrame.top + 1 ), Point( cTabFrame.right - 1, cTabFrame.top + 1 ) );
		}

		if( i != m_pcParent->m_nSelectedTab - 1 )
		{
			SetFgColor( sBgShadowCol );
			DrawLine( Point( cTabFrame.right, cTabFrame.top + 2 ), Point( cTabFrame.right, cTabFrame.bottom - 2 ) );

			SetFgColor( sBgCol );
			DrawLine( Point( cTabFrame.right - 1, cTabFrame.top + 2 ), Point( cTabFrame.right - 1, cTabFrame.bottom - 1 ) );
		}
		SetFgColor( 0, 0, 0 );

		MovePenTo( x + ( vWidth - 4 ) * 5 / 100 + 4, cTabFrame.top + vTabHeight / 2 + sFontHeight.ascender - sFontHeight.line_gap / 2 - vGlyphHeight / 2 + 2 );

		DrawString( m_pcParent->m_cTabList[i].m_cTitle.c_str() );
		x += vWidth;
	}
}

void TabView::Paint( const Rect & cUpdateRect )
{
	Rect cBounds = GetBounds();

	cBounds.Floor();

	float nViewWidth = cBounds.Width() + 1.0f;

	Rect cTabRect = cBounds;

	cTabRect.bottom = m_vTabHeight - 3;
	cTabRect.Floor();

	cBounds.top += m_vTabHeight - 2;

	cBounds.Floor();

	Color32_s sFgCol( get_default_color( COL_SHINE ) );
	Color32_s sBgCol( get_default_color( COL_SHADOW ) );

	Color32_s sFgShadowCol = Tint( sFgCol, 0.9f );
	Color32_s sBgShadowCol = Tint( sBgCol, 0.8f );

	SetFgColor( sBgShadowCol );
	DrawLine( Point( cBounds.right, cBounds.top + 2 ), Point( cBounds.right, cBounds.bottom ) );
	DrawLine( Point( cBounds.left, cBounds.bottom ) );

	SetFgColor( sBgCol );

	DrawLine( Point( cBounds.right - 1, cBounds.top + 2 ), Point( cBounds.right - 1, cBounds.bottom - 1 ) );
	DrawLine( Point( cBounds.left + 1, cBounds.bottom - 1 ) );

	SetFgColor( sFgShadowCol );
	DrawLine( Point( cBounds.left, cBounds.top - 1 ), Point( cBounds.left, cBounds.bottom - 2 ) );

	SetFgColor( sFgCol );
	DrawLine( Point( cBounds.left + 1, cBounds.top + 1 ), Point( cBounds.left + 1, cBounds.bottom - 2 ) );

	float x = m_vScrollOffset;

	if( m_vTotalWidth > nViewWidth )
	{
		x += 8;
	}
	if( m_vTotalWidth <= nViewWidth )
	{
		SetFgColor( get_default_color( COL_NORMAL ) );
		FillRect( Rect( 0, 0, 1, m_vTabHeight - 3 ) );
		FillRect( Rect( cTabRect.right - 1, 0, cTabRect.right, m_vTabHeight - 3 ) );
	}

	for( uint i = 0; i < m_cTabList.size(); ++i )
	{
		float vWidth = m_cTabList[i].m_vWidth;
		Rect cTabFrame( x + 2, 2, x + vWidth - 1 + 2, m_vTabHeight - 1 );

		cTabFrame.Floor();
		if( i == m_nSelectedTab )
		{
			cTabFrame.Resize( -2, -2, 2, 0 );
		}

		if( m_vTotalWidth <= nViewWidth )
		{
			if( i != m_nSelectedTab + 1 )
			{
				SetFgColor( sFgShadowCol );
				DrawLine( Point( cTabFrame.left, cTabFrame.bottom - 2 ), Point( cTabFrame.left, cTabFrame.top + 3 ) );
				DrawLine( Point( cTabFrame.left + 3, cTabFrame.top ) );
				SetFgColor( sFgCol );
				DrawLine( Point( cTabFrame.left + 1, cTabFrame.bottom - 1 ), Point( cTabFrame.left + 1, cTabFrame.top + 4 ) );
				DrawLine( Point( cTabFrame.left + 4, cTabFrame.top + 1 ) );
			}
		}
		else
		{
			if( i == uint ( m_nSelectedTab ) )
			{
				DrawLine( Point( cTabFrame.left + 1, cTabFrame.bottom - 1 ), Point( cTabFrame.left + 1, cTabFrame.bottom - 1 ) );
			}
		}
		if( i == uint ( m_nSelectedTab ) )
		{
			SetFgColor( get_default_color( COL_NORMAL ) );
			float x1 = x + 2.0f;
			float x2 = x + vWidth + 2.0f;

			if( m_vTotalWidth > nViewWidth )
			{
				if( x1 < 8 )
				{
					x1 = 8;
				}
				if( x2 > cBounds.right - 9.0f )
				{
					x2 = cBounds.right - 9.0f;
				}
			}
			DrawLine( Point( x1, cTabFrame.bottom - 1 ), Point( x2, cTabFrame.bottom - 1 ) );
			DrawLine( Point( x1, cTabFrame.bottom ), Point( x2, cTabFrame.bottom ) );

			SetFgColor( sFgShadowCol );

			if( x >= 0 )
			{
				DrawLine( Point( 0, cTabFrame.bottom - 1 ), Point( x, cTabFrame.bottom - 1 ) );
			}
			DrawLine( Point( x + vWidth + 2, cTabFrame.bottom - 1 ), Point( cBounds.right, cTabFrame.bottom - 1 ) );

			SetFgColor( sFgCol );
			if( x >= 1 )
			{
				DrawLine( Point( 1, cTabFrame.bottom ), Point( x + 1, cTabFrame.bottom ) );
			}
			DrawLine( Point( x + vWidth + 2, cTabFrame.bottom ), Point( cBounds.right, cTabFrame.bottom ) );
		}
		x += vWidth;
	}

	if( m_vTotalWidth > nViewWidth )
	{
		SetFgColor( get_default_color( COL_NORMAL ) );
		FillRect( Rect( 0, 0, 7, m_vTabHeight - 3 ) );
		FillRect( Rect( cTabRect.right - 7, 0, cTabRect.right, m_vTabHeight - 3 ) );
		SetFgColor( sBgCol );
		for( int x = 2; x < 6; ++x )
		{
			DrawLine( Point( x, m_vTabHeight / 2 - ( x - 2 ) ), Point( x, m_vTabHeight / 2 + ( x - 2 ) ) );
			DrawLine( Point( cBounds.right - x, m_vTabHeight / 2 - ( x - 2 ) ), Point( cBounds.right - x, m_vTabHeight / 2 + ( x - 2 ) ) );
		}

		SetFgColor( sFgShadowCol );
		DrawLine( Point( 0, m_vTabHeight - 2 ), Point( 8, m_vTabHeight - 2 ) );
		DrawLine( Point( cBounds.right - 8, m_vTabHeight - 2 ), Point( cBounds.right, m_vTabHeight - 2 ) );
		SetFgColor( sFgCol );
		DrawLine( Point( 1, m_vTabHeight - 1 ), Point( 8, m_vTabHeight - 1 ) );
		DrawLine( Point( cBounds.right - 8, m_vTabHeight - 1 ), Point( cBounds.right, m_vTabHeight - 1 ) );
	}
}
