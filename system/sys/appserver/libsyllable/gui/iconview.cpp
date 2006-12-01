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
 
#include <atheos/time.h>
#include <gui/image.h>
#include <gui/font.h>
#include <gui/rect.h>
#include <gui/scrollbar.h>
#include <gui/guidefines.h>
#include <gui/iconview.h>
#include <gui/desktop.h>
#include <util/resources.h>
#include <util/looper.h>
#include <util/application.h>
#include <appserver/protocol.h>
#include <vector>
#include <iostream>

using namespace os;

class Icon
{
public:
	os::Point  			m_cPosition;
	std::vector<os::String>	m_zStrings;
	os::Image* 	m_pcImage;
	os::IconData* 		m_pcData;
	
	bool				m_bSelected;
	bool				m_bLayouted;
};

struct IconSort
{
	bool operator() ( const Icon * x, const Icon * y ) const
	{
		if( x->m_zStrings.size() < 1 || y->m_zStrings.size() < 1 )
			return( false );
		
		
		
		return ( x->m_zStrings[0] < y->m_zStrings[0] );
	}
};

class IconView::Private
{
public:
	Private( os::IconView* pcControl )

	{
		/* Set defaults */
		m_eType = VIEW_ICONS;
		m_pcControl = pcControl;
		m_bMouseDown = false;
		m_bMouseDownOverIcon = false;
		m_bSelecting = false;
		m_bDragging = false;
		m_vLastXPos = 0;
		m_vLastYPos = 0;
		m_nLastClick = 0;
		m_pcSelChangeMsg = NULL;
		m_bScrollDown = false;
		m_bScrollUp = false;
		m_bAdjusting = false;
		m_pcBackground = NULL;
		m_sTextColor = get_default_color(COL_ICON_TEXT);  //os::Color32_s( 0, 0, 0 );
		m_sTextShadowColor = get_default_color(COL_ICON_TEXT);
		m_sBackgroundColor = get_default_color(COL_ICON_BG); //os::Color32_s( 255, 255, 255 );
		m_sSelectionColor = get_default_color(COL_ICON_SELECTED);  //os::Color32( 186, 199, 227 );
		m_cIcons.clear();
		m_pcIconLock = new os::Locker( "iconview_lock" );
		m_bSingleClick = false;
		m_bAutoSort = true;
		m_bMultiSelect = true;
		m_bHScrollBarVisible = true;
		m_bVScrollBarVisible = true;
	}
	
	void Lock()
	{
		m_pcIconLock->Lock();
	}
	
	void Unlock()
	{
		m_pcIconLock->Unlock();
	}
	
	/* Calculate the biggest icon size */
	void CalculateMaxIconSize()
	{
		m_vIconWidth = 0;
		m_vIconHeight = 0;
		
		Lock();
		if( m_cIcons.size() == 0 ) {
			Unlock();
			return;
		}
		Unlock();
			
		
		/* Calculate string widths */
		if( m_eType == VIEW_DETAILS )
		{
			Lock();
			for( uint j = 0; j < m_cIcons[0]->m_zStrings.size(); j++ )
			{
				m_vStringWidth[j] = 0;
				for( uint i = 0; i < m_cIcons.size(); i++ )
				{
					if( !m_cIcons[i]->m_zStrings[j].empty() ) {
						float vCurrentW = m_pcView->GetStringWidth( m_cIcons[i]->m_zStrings[j] );
						m_vStringWidth[j] = std::max( m_vStringWidth[j], vCurrentW );
					}
				}
			}
			Unlock();
		}
		
			
		/* Calculate font height */
		os::font_height sHeight;
		m_pcView->GetFontHeight( &sHeight );
		float vFontHeight = sHeight.ascender + sHeight.descender;
		
		Lock();
		for( uint i = 0; i < m_cIcons.size(); i++ )
		{
			float vW = 0;
			float vH = 0;
			
			/* Calculate font size */
			if( m_eType == VIEW_DETAILS ) {
				for( uint j = 0; j < m_cIcons[i]->m_zStrings.size(); j++ )
				{
				
					vW += m_vStringWidth[j] + 10;
				}
			} else {
				vW = m_pcView->GetStringWidth( m_cIcons[i]->m_zStrings[0] );
			}
		
			/* Add bitmap size */
			if( m_eType == VIEW_ICONS || m_eType == VIEW_ICONS_DESKTOP ) {
				vW = std::max( vW, m_cIcons[i]->m_pcImage->GetSize().x );
				vH = vFontHeight + m_cIcons[i]->m_pcImage->GetSize().y + 10;
			} else {
				vW = vW + m_cIcons[i]->m_pcImage->GetSize().x + 10;
				vH = std::max( vFontHeight, m_cIcons[i]->m_pcImage->GetSize().y );
			}
		
			
			/* Compare size */
			m_vIconWidth = std::max( vW, m_vIconWidth );
			m_vIconHeight = std::max( vH, m_vIconHeight );
		}
		Unlock();
	}
	
	/* Sort icons */
	void SortIcons()
	{
		if( !m_bAutoSort )
			return;
		
		Lock();
		if( m_cIcons.size() == 0 ) {
			Unlock();
			return;
		}
		for( uint i = 0; i < m_cIcons.size(); i++ )
		{
			std::sort( m_cIcons.begin(), m_cIcons.end(  ), IconSort() );
		}
		Unlock();
	}
	
	/* Returns the frame in which we draw the icons */
	inline os::Rect GetViewFrame()
	{
		os::Rect cViewFrame = m_pcView->GetBounds() + m_pcView->GetScrollOffset();
		if( m_eType == VIEW_ICONS_DESKTOP )
		{
			os::Message cReq( os::DR_GET_DESKTOP_MAX_WINFRAME );
			os::Message cReply;
			cReq.AddInt32( "desktop", os::Desktop::ACTIVE_DESKTOP );
			os::Application* pcApp = os::Application::GetInstance();
			os::Messenger( pcApp->GetServerPort() ).SendMessage( &cReq, &cReply );
			cReply.FindRect( "frame", &cViewFrame );
		}
		return( cViewFrame );
	}
	
	/* Layout the icons */
	void LayoutIcons()
	{
		
		CalculateMaxIconSize();
		
		if( m_vIconWidth == 0 ) {
			m_vLastXPos = 0;
			m_vLastYPos = 0;
			AdjustScrollBars();
			m_pcView->Invalidate();
			m_pcView->Flush();
			return;
		}
		
		os::Rect cViewFrame = GetViewFrame();
		
		/* Calculate icons per row */
		if( m_eType == VIEW_ICONS || m_eType == VIEW_DETAILS )
			m_nIconsPerRow = (int)cViewFrame.Width() / (int)( m_vIconWidth + 6 );
		else
			m_nIconsPerRow = (int)cViewFrame.Height() / (int)( m_vIconHeight + 6 );
		m_nIconsPerRow = std::max( m_nIconsPerRow, 1 );
		
		float vX = cViewFrame.left + 5;
		float vY = cViewFrame.top + 5;
		int nCurrent = 0;
		
		/* Assign positions */
		Lock();
		for( uint i = 0; i < m_cIcons.size(); i++ )
		{
			m_cIcons[i]->m_cPosition = os::Point( vX, vY );
			if( m_eType == VIEW_ICONS )
				vX += m_vIconWidth + 6;
			else
				vY += m_vIconHeight + 6;
			nCurrent++;
			if( nCurrent == m_nIconsPerRow && m_eType != VIEW_DETAILS )
			{
				if( m_eType == VIEW_ICONS ) {
					vX = cViewFrame.left + 5;
					vY += m_vIconHeight + 6;
				} else {
					vX += m_vIconWidth + 6;
					vY = cViewFrame.top + 5;
				}
				nCurrent = 0;
			}
			m_cIcons[i]->m_bLayouted = true;
		}
		Unlock();
		
		/* Update scrollbar */
		m_vLastXPos = vX + ( ( nCurrent == 0 ) ? 0 : m_vIconWidth );
		m_vLastYPos = vY + ( ( ( nCurrent == 0 ) || m_eType == VIEW_DETAILS ) ? 0 : m_vIconHeight );
		m_pcHScrollBar->SetMinMax( 0, m_vLastXPos - cViewFrame.Width() );
		m_pcVScrollBar->SetMinMax( 0, m_vLastYPos - cViewFrame.Height() );
		AdjustScrollBars();
		m_pcHScrollBar->SetProportion( cViewFrame.Width() / m_vLastXPos );
		m_pcHScrollBar->SetValue( 0 );
		m_pcVScrollBar->SetProportion( cViewFrame.Height() / m_vLastYPos );
		m_pcVScrollBar->SetValue( 0 );

		/* Update view */
		m_pcView->Invalidate();
		m_pcView->Flush();
	}
	
	/* Relayout the icons only if the number of icons per row has changed */
	void LayoutIconsIfNecessary()
	{
		if( m_vIconWidth == 0 )
			return;
			
		os::Rect cViewFrame = GetViewFrame();
		
		/* Look if the number of icons in one row has changed */
		int nIconsPerRow = 0;
		if( m_eType == VIEW_ICONS || m_eType == VIEW_DETAILS )
			nIconsPerRow = (int)cViewFrame.Width() / (int)( m_vIconWidth + 6 );
		else
			nIconsPerRow = (int)cViewFrame.Height() / (int)( m_vIconHeight + 6 );
		nIconsPerRow = std::max( nIconsPerRow, 1 );
		
		AdjustScrollBars();
		m_pcHScrollBar->SetValue( 0 );
		m_pcHScrollBar->SetMinMax( 0, m_vLastXPos - cViewFrame.Width() );
		m_pcHScrollBar->SetProportion( cViewFrame.Width() / m_vLastXPos );
		m_pcVScrollBar->SetValue( 0 );
		m_pcVScrollBar->SetMinMax( 0, m_vLastYPos - cViewFrame.Height() );
		m_pcVScrollBar->SetProportion( cViewFrame.Height() / m_vLastYPos );
		
		if( nIconsPerRow != m_nIconsPerRow )
			LayoutIcons();
	}
	
	/* Return what icon is under the cursor */
	int HitTest( const os::Point& cPosition )
	{
		/* Assign positions */
		Lock();
		for( uint i = 0; i < m_cIcons.size(); i++ )
		{
			if( !m_cIcons[i]->m_bLayouted )
				continue;
				
			os::Point cIconPos;
			os::Point cIconSize;
			if( m_eType == VIEW_ICONS || m_eType == VIEW_ICONS_DESKTOP ) {
				cIconPos = os::Point( m_cIcons[i]->m_cPosition.x + 
										( m_vIconWidth - m_cIcons[i]->m_pcImage->GetSize().x ) / 2, 
										m_cIcons[i]->m_cPosition.y );
				cIconSize = os::Point( m_cIcons[i]->m_pcImage->GetSize().x, m_cIcons[i]->m_pcImage->GetSize().y );
			} else {
				cIconPos = m_cIcons[i]->m_cPosition;
				cIconSize = os::Point( m_vIconWidth, m_cIcons[i]->m_pcImage->GetSize().y );
			}
			
			/* Create frame */
			os::Rect cFrame( cIconPos, cIconPos + cIconSize );
			
			if( cFrame.DoIntersect( cPosition ) )
			{
				Unlock();
				return( i );
			}
		}
		Unlock();
		return( -1 );
	}
	
	/* Deselect all icons */
	void DeselectAll()
	{
		Lock();
		for( uint i = 0; i < m_cIcons.size(); i++ )
		{
			os::Rect cIconFrame( m_cIcons[i]->m_cPosition - os::Point( 3, 3 ), m_cIcons[i]->m_cPosition
											+ os::Point( m_vIconWidth, m_vIconHeight ) + os::Point( 3, 3 ) );
			if( m_cIcons[i]->m_bSelected && m_pcView->GetBounds().DoIntersect( cIconFrame ) )
				m_pcView->Invalidate( cIconFrame );
			m_cIcons[i]->m_bSelected = false;
		}
		Unlock();
	}
	
	/* Select one Icon */
	void Select( uint nIcon, bool bSelected )
	{
		Lock();
		if( m_cIcons[nIcon]->m_bSelected == bSelected )
		{
			Unlock();
			return;
		}
		m_cIcons[nIcon]->m_bSelected = bSelected;
		m_pcView->Invalidate( os::Rect( m_cIcons[nIcon]->m_cPosition - os::Point( 3, 3 ), m_cIcons[nIcon]->m_cPosition
											+ os::Point( m_vIconWidth, m_vIconHeight ) + os::Point( 3, 3 ) ) );
		Unlock();	
	}

	
	/* Select the icons in this region */
	void Select( os::Point cStart, os::Point cEnd, bool bKeepSelection )
	{
		
		if( !bKeepSelection )
			DeselectAll();
			
		/* Swap positions if necessary */
		if( cStart.x > cEnd.x ) {
			float vTemp = cStart.x;
			cStart.x = cEnd.x;
			cEnd.x = vTemp;
		}	
		if( cStart.y > cEnd.y ) {
			float vTemp = cStart.y;
			cStart.y = cEnd.y;
			cEnd.y = vTemp;
		}				
				
		/* Create frame */
		os::Rect cSelectFrame( cStart, cEnd );
		
		Lock();
		
		/* Check positions */
		for( uint i = 0; i < m_cIcons.size(); i++ )
		{
			if( !m_cIcons[i]->m_bLayouted )
				continue;
			os::Point cIconPos = os::Point( m_cIcons[i]->m_cPosition.x + 
										( m_vIconWidth - m_cIcons[i]->m_pcImage->GetSize().x ) / 2, 
										m_cIcons[i]->m_cPosition.y );
			
			/* Create frame */
			os::Rect cFrame( cIconPos, cIconPos + m_cIcons[i]->m_pcImage->GetSize() );
			
			/* Select/deselect */
			if( cFrame.DoIntersect( cSelectFrame ) )
			{
				m_cIcons[i]->m_bSelected = !m_cIcons[i]->m_bSelected;
			}
		}
		Unlock();
	}
	
	/* Select next or previous icon */
	void SelectNextPrev( bool bNext )
	{
		uint nSelected = 0;
		bool bFoundSelected = false;
		if( m_eType != VIEW_DETAILS )
			return;
			
		Lock();
		if( m_cIcons.size() == 0 ) {
			Unlock();
			return;
		}
		for( uint i = 0; i < m_cIcons.size(); i++ )
		{
			/* Find the first selected icon */
			if( m_cIcons[i]->m_bSelected )
			{
				nSelected = i;
				bFoundSelected = true;
				break;
			}
		}
		if( !bFoundSelected )
		{
			nSelected = 0;
		} else {
			if( bNext ) {
				if( nSelected == m_cIcons.size() - 1 )
					nSelected = 0;
				else
					nSelected++;
			} else {
				if( nSelected == 0 )
					nSelected = m_cIcons.size() - 1;
				else
					nSelected--;
			}
		}
		DeselectAll();
		Select( nSelected, true );
		
		/* Scroll to make the icon visible */
		os::Rect cFrame = m_pcView->GetBounds();
		os::Rect cIconFrame( m_cIcons[nSelected]->m_cPosition - os::Point( 3, 3 ), m_cIcons[nSelected]->m_cPosition +
																os::Point( m_vIconWidth, m_vIconHeight ) + os::Point( 3, 3 ) );
		if( !( cFrame.DoIntersect( os::Point( 0, cIconFrame.top ) ) && cFrame.DoIntersect( os::Point( 0, cIconFrame.bottom ) ) ) && !( m_bScrollDown || m_bScrollUp ) )
		{
			float vScroll;
			if( bNext )
				vScroll = -( cIconFrame.top - m_pcView->GetBounds().Height() + cIconFrame.Height() );
			else
				vScroll = -( cIconFrame.top );
			if( vScroll > 0 )
				vScroll = 0;
			if( vScroll < -( m_vLastYPos - cFrame.Height() ) )
				vScroll = -( m_vLastYPos - cFrame.Height() );
			m_pcView->ScrollTo( os::Point( 0, vScroll ) );
		}

		Unlock();
	}

	
	/* Render selection */
	void RenderSelection( uint nIcon, os::View* pcView, os::Point cPosition )
	{
		os::Rect cFrame( cPosition, cPosition + os::Point( m_vIconWidth, m_vIconHeight ) );
		Lock();
		if( m_cIcons[nIcon]->m_bSelected )
		{
			os::Rect cSelectFrame = cFrame;
			
			pcView->FillRect( cSelectFrame );
			
			/* Round edges */
			pcView->DrawLine( os::Point( cSelectFrame.left + 2, cSelectFrame.top - 2 ), 
								os::Point( cSelectFrame.right - 2, cSelectFrame.top - 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left, cSelectFrame.top - 1 ), 
								os::Point( cSelectFrame.right, cSelectFrame.top - 1 ) );
			
			pcView->DrawLine( os::Point( cSelectFrame.left - 2, cSelectFrame.top + 2 ), 
								os::Point( cSelectFrame.left - 2, cSelectFrame.bottom - 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left - 1, cSelectFrame.top ), 
								os::Point( cSelectFrame.left - 1, cSelectFrame.bottom ) );
								
			pcView->DrawLine( os::Point( cSelectFrame.left + 2, cSelectFrame.bottom + 2 ), 
								os::Point( cSelectFrame.right - 2, cSelectFrame.bottom + 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.left, cSelectFrame.bottom + 1 ), 
								os::Point( cSelectFrame.right, cSelectFrame.bottom + 1 ) );
								
			pcView->DrawLine( os::Point( cSelectFrame.right + 2, cSelectFrame.top + 2 ), 
								os::Point( cSelectFrame.right + 2, cSelectFrame.bottom - 2 ) );
			pcView->DrawLine( os::Point( cSelectFrame.right + 1, cSelectFrame.top ), 
								os::Point( cSelectFrame.right + 1, cSelectFrame.bottom ) );
		}
		Unlock();
	}
	
	/* Render one icon into a view */
	void RenderIcon( uint nIcon, os::View* pcView, os::Point cPosition )
	{
		os::Rect cFrame( cPosition, cPosition + os::Point( m_vIconWidth, m_vIconHeight ) );
		
		Lock();
		
		if( m_cIcons[nIcon]->m_bSelected )
		{
			pcView->SetBgColor( get_default_color( os::COL_SEL_MENU_BACKGROUND ) );
		} else {
			pcView->SetBgColor( m_sBackgroundColor );
		}
				
				
		/* Draw Icon */
		pcView->SetDrawingMode( os::DM_BLEND );
		//std::cout<<vIconWidth<<" "<<GetSize().x<<" "<<m_cPosition.x + ( vIconWidth - GetSize().x ) / 2<<std::endl;
		if( m_eType == VIEW_ICONS || m_eType == VIEW_ICONS_DESKTOP )
			m_cIcons[nIcon]->m_pcImage->Draw( os::Point( cPosition.x + 
										( m_vIconWidth - m_cIcons[nIcon]->m_pcImage->GetSize().x ) / 2, 
											cPosition.y ), pcView );
		else
			m_cIcons[nIcon]->m_pcImage->Draw( os::Point( cPosition.x , cPosition.y + 
										( m_vIconHeight - m_cIcons[nIcon]->m_pcImage->GetSize().y ) / 2 ), pcView );									
					
		/* Draw Text */
		os::font_height sHeight;
		pcView->GetFontHeight( &sHeight );
		pcView->SetDrawingMode( os::DM_OVER );
		if( m_eType == VIEW_ICONS || m_eType == VIEW_ICONS_DESKTOP ) {
			if( m_cIcons[nIcon]->m_zStrings.size() > 0 ) {
				if ( ( m_eType == VIEW_ICONS_DESKTOP ) && (m_sTextColor!=m_sTextShadowColor ) ) {
						pcView->SetFgColor(m_sTextShadowColor);
						pcView->DrawString( os::Point( cPosition.x + 1 + ( m_vIconWidth -
									pcView->GetStringWidth( m_cIcons[nIcon]->m_zStrings[0] ) ) / 2, cPosition.y + m_vIconHeight + 1
									- sHeight.ascender ), m_cIcons[nIcon]->m_zStrings[0] );
						pcView->SetFgColor(m_sTextColor);
				}

				pcView->DrawString( os::Point( cPosition.x + ( m_vIconWidth - 
								pcView->GetStringWidth( m_cIcons[nIcon]->m_zStrings[0] ) ) / 2, cPosition.y + m_vIconHeight
								- sHeight.ascender ), m_cIcons[nIcon]->m_zStrings[0] );
		
			}
					
		} else if( m_eType == VIEW_DETAILS ) {
			float vCurrentW = cPosition.x + m_cIcons[nIcon]->m_pcImage->GetSize().x + 5;
			for( uint i = 0; i < m_cIcons[nIcon]->m_zStrings.size(); i++ )
			{
				pcView->DrawString( os::Point( vCurrentW, cPosition.y + ( m_vIconHeight - 
								sHeight.ascender - sHeight.descender ) / 2
								+ sHeight.ascender ), m_cIcons[nIcon]->m_zStrings[i] );
				vCurrentW += m_vStringWidth[i] + 10;
				if( i == 0 )
					pcView->SetFgColor( get_default_color( os::COL_SHADOW ) );
			}
			pcView->SetFgColor( 0, 0, 0 );
		} else {
			if( m_cIcons[nIcon]->m_zStrings.size() > 0 )
				pcView->DrawString( os::Point( cPosition.x + m_cIcons[nIcon]->m_pcImage->GetSize().x + 5, cPosition.y + ( m_vIconHeight - 
								sHeight.ascender - sHeight.descender ) / 2
								+ sHeight.ascender ), m_cIcons[nIcon]->m_zStrings[0] );
		}
		Unlock();
	}
	
	/* Render one icon into a view */
	void RenderIcon( os::String zName, os::Image* pcImage, os::View* pcView, os::Point cPosition )
	{
		os::Rect cFrame( cPosition, cPosition + os::Point( m_vIconWidth, m_vIconHeight ) );
		pcView->SetBgColor( m_sBackgroundColor );
		
		
		/* Draw Icon */
		pcView->SetDrawingMode( os::DM_COPY );
		//std::cout<<vIconWidth<<" "<<GetSize().x<<" "<<m_cPosition.x + ( vIconWidth - GetSize().x ) / 2<<std::endl;
		if( m_eType == VIEW_ICONS || m_eType == VIEW_ICONS_DESKTOP )
			pcImage->Draw( os::Point( cPosition.x + 
										( m_vIconWidth - pcImage->GetSize().x ) / 2, 
										cPosition.y ), pcView );
		else
			pcImage->Draw( os::Point( cPosition.x , cPosition.y + 
										( m_vIconHeight - pcImage->GetSize().y ) / 2 ), pcView );									
					
		/* Draw Text */
		os::font_height sHeight;
		pcView->GetFontHeight( &sHeight );
		pcView->SetFgColor( m_sTextColor );
		pcView->SetDrawingMode( os::DM_BLEND );
		if( m_eType == VIEW_ICONS || m_eType == VIEW_ICONS_DESKTOP )
			pcView->DrawString( os::Point( cPosition.x + ( m_vIconWidth - 
								pcView->GetStringWidth( zName ) ) / 2, cPosition.y + m_vIconHeight
								- sHeight.ascender ), zName );
		else
			pcView->DrawString( os::Point( cPosition.x + pcImage->GetSize().x + 5, cPosition.y + ( m_vIconHeight - 
								sHeight.ascender - sHeight.descender  ) / 2
								+ sHeight.ascender ), zName );
	}
	
	void AdjustScrollBars()
	{
		if( m_bAdjusting )
			return;
		m_bAdjusting = true;
		
		if( m_vLastYPos > m_pcView->GetBounds().Height() && !m_bVScrollBarVisible 
			&& ( m_eType == VIEW_ICONS || m_eType == VIEW_DETAILS ) )
		{
			os::Rect cFrame = m_pcView->GetFrame();
			cFrame.right -= m_vScrollBarWidth;
			m_pcView->SetFrame( cFrame );
			m_pcVScrollBar->Show();
			m_bVScrollBarVisible = true;
		}
		else if( m_vLastYPos <= m_pcView->GetBounds().Height() && m_bVScrollBarVisible ) 
		{
			m_pcVScrollBar->Hide();
			m_bVScrollBarVisible = false;
			os::Rect cFrame = m_pcView->GetFrame();
			cFrame.right += m_vScrollBarWidth;
			m_pcView->SetFrame( cFrame );
		}
		if( m_vLastXPos > m_pcView->GetBounds().Width() && !m_bHScrollBarVisible 
			&& ( m_eType == VIEW_LIST || m_eType == VIEW_ICONS_DESKTOP ) )
		{
			os::Rect cFrame = m_pcView->GetFrame();
			cFrame.bottom -= m_vScrollBarHeight;
			m_pcView->SetFrame( cFrame );
			m_pcHScrollBar->Show();
			m_bHScrollBarVisible = true;
		}
		else if( m_vLastXPos <= m_pcView->GetBounds().Width() && m_bHScrollBarVisible ) 
		{
			m_pcHScrollBar->Hide();
			m_bHScrollBarVisible = false;
			os::Rect cFrame = m_pcView->GetFrame();
			cFrame.bottom += m_vScrollBarHeight;
			m_pcView->SetFrame( cFrame );
		}
		m_bAdjusting = false;
	}
	
	
	os::IconView* m_pcControl;
	os::View* m_pcView;
	os::ScrollBar* m_pcHScrollBar;
	os::ScrollBar* m_pcVScrollBar;
	os::Locker* m_pcIconLock;
	std::vector<Icon*> m_cIcons;
	view_type m_eType;
	os::Image* m_pcBackground;
	os::Color32_s m_sBackgroundColor;
	os::Color32_s m_sTextColor;
	os::Color32_s m_sTextShadowColor;	
	os::Color32_s m_sSelectionColor;
	
	float m_vIconWidth;
	float m_vIconHeight;
	float m_vStringWidth[10];
	int m_nIconsPerRow;
	bool m_bMouseDown;
	bool m_bMouseDownOverIcon;
	os::Point m_cMouseDownPos;
	bool m_bMouseSelectedIcon;
	bool m_bDragging;
	bool m_bSelecting;
	float m_vLastXPos;
	float m_vLastYPos;
	os::Point m_cSelectStart;
	os::Point m_cLastSelectPosition;
	bigtime_t m_nLastClick;
	os::Message* m_pcSelChangeMsg;
	bool m_bAdjusting;
	bool m_bScrollDown;
	bool m_bScrollUp;
	float m_vScrollBarWidth;
	float m_vScrollBarHeight;
	bool m_bSingleClick;
	bool m_bAutoSort;
	bool m_bMultiSelect;
	bool m_bVScrollBarVisible;
	bool m_bHScrollBarVisible;
};

class IconView::MainView : public os::View
{
public:
	MainView( os::Rect cFrame, IconView::Private* pcPrivate ) : os::View( cFrame, "filesystem_view", os::CF_FOLLOW_ALL, os::WID_WILL_DRAW )
	{
		m = pcPrivate;
	}
	~MainView()
	{
	}
	
	
	void Paint( const os::Rect& cUpdateRect )
	{
		//printf( "Paint %f %f %f %f\n", cUpdateRect.left, cUpdateRect.top, cUpdateRect.right, cUpdateRect.bottom );
		
		/* Draw background */
		SetFgColor( m->m_sBackgroundColor );
		SetDrawingMode( os::DM_COPY );
		
		if( m->m_pcBackground && os::Rect( os::Point( 0, 0 ), m->m_pcBackground->GetSize() ).
			DoIntersect( cUpdateRect ) )
		{
			
			os::Rect cBitmapRect = cUpdateRect & os::Rect( os::Point( 0, 0 ), m->m_pcBackground->GetSize() - os::Point( 1, 1 ) );
			
			m->m_pcBackground->Draw( cBitmapRect, cBitmapRect, this );
		} else
			FillRect( cUpdateRect );
		
		
		/* Draw icons and selection */
		SetFgColor( m->m_sSelectionColor );
		m->Lock();
		for( uint i = 0; i < m->m_cIcons.size(); i++ )
		{
			if( !m->m_cIcons[i]->m_bLayouted )
				continue;
			
			os::Rect cFrame( m->m_cIcons[i]->m_cPosition - os::Point( 3, 3 ), m->m_cIcons[i]->m_cPosition +
																os::Point( m->m_vIconWidth, m->m_vIconHeight ) + os::Point( 3, 3 ) );
			
			/* Render a gray background in details mode */			
			if( m->m_eType == VIEW_DETAILS && ( i & 1 ) ) {
				os::Rect cGray = cFrame;
				cGray.right = GetBounds().right;
				cGray.left = 0;
				FillRect( cGray, Color32_s( 240, 240, 240 ) );
			}
			
			if( cUpdateRect.DoIntersect( cFrame ) )
			{
				/* Draw icon background if selected */
				m->RenderSelection( i, this, m->m_cIcons[i]->m_cPosition );
			}
		}
		SetDrawingMode( os::DM_COPY );
		SetFgColor( m->m_sTextColor );
		for( uint i = 0; i < m->m_cIcons.size(); i++ )
		{
			if( !m->m_cIcons[i]->m_bLayouted )
				continue;
			
			os::Rect cFrame( m->m_cIcons[i]->m_cPosition - os::Point( 3, 3 ), m->m_cIcons[i]->m_cPosition +
																os::Point( m->m_vIconWidth, m->m_vIconHeight ) + os::Point( 3, 3 ) );
			if( cUpdateRect.DoIntersect( cFrame ) )
			{
				/* Draw icon  */
				m->RenderIcon( i, this, m->m_cIcons[i]->m_cPosition );
			}
		}
		/* Render selection rectangle */
		if( m->m_bSelecting )
		{
			os::Color32_s sColor = m->m_sSelectionColor;
			sColor.alpha = 128;
			
			os::Rect cSelect( m->m_cSelectStart, m->m_cLastSelectPosition );
			float vTemp;
			if( cSelect.left > cSelect.right ) {
				vTemp = cSelect.right;
				cSelect.right = cSelect.left;
				cSelect.left = vTemp;
			}
			if( cSelect.top > cSelect.bottom ) {
				vTemp = cSelect.bottom;
				cSelect.bottom = cSelect.top;
				cSelect.top = vTemp;
			}
			SetDrawingMode( os::DM_COPY );
			SetFgColor( sColor );
			DrawLine( os::Point( cSelect.left, cSelect.top ), os::Point( cSelect.right, cSelect.top ) );
			DrawLine( os::Point( cSelect.right, cSelect.top ), os::Point( cSelect.right, cSelect.bottom ) );
			DrawLine( os::Point( cSelect.left, cSelect.bottom ), os::Point( cSelect.right, cSelect.bottom ) );
			DrawLine( os::Point( cSelect.left, cSelect.top ), os::Point( cSelect.left, cSelect.bottom ) );
			SetDrawingMode( os::DM_BLEND );
			cSelect.Resize( 1, 1, -1, -1 );
			FillRect( cSelect );
		}
		m->Unlock();
	}
	
	void WindowActivated( bool bFocus )
	{
		if( bFocus )
			return;
		
		if( m->m_bSelecting )
		{
			Invalidate();
			Flush();
		}
		
		/* Reset values */
		m->m_bMouseDown = false;
		m->m_bMouseDownOverIcon = false;
		m->m_bSelecting = false;
		m->m_bDragging = false;
		m->m_bMouseSelectedIcon = false;
	}
	
	void MouseDown( const os::Point& cPosition, uint32 nButtons )
	{
		int nIcon = 0;
		
		AutoLocker cLocker( m->m_pcIconLock );
		
		MakeFocus();
		
		m->m_bMouseDown = true;
		m->m_bSelecting = false;
		m->m_cMouseDownPos = cPosition;
		m->m_bMouseSelectedIcon = false;
		
		if( ( nIcon = m->HitTest( cPosition ) ) < 0 )
		{
			/* Deselect all icons if ctrl is not pressed */
			if( !( GetQualifiers() & os::QUAL_CTRL ) )
			{
				m->DeselectAll();
				m->m_pcControl->SelectionChanged();
				Flush();
			}
			m->m_cSelectStart = cPosition;
			
			if( nButtons == 1 )
				return( os::View::MouseDown( cPosition, nButtons ) );
		}
		else
		{
			/* Select icon if we hit one */
			if( !m->m_cIcons[nIcon]->m_bSelected ) {
				if( !( ( GetQualifiers() & os::QUAL_CTRL ) && m->m_bMultiSelect ) )
					/* Deselect all icons */
					m->DeselectAll();		
				m->Select( nIcon, true );
				m->m_bMouseSelectedIcon = true;
			}
			
			m->m_pcControl->SelectionChanged();
			Flush();
			
			
		}
		
		/* Open context menu */
		if( nButtons != 1 )
		{
			m->m_pcControl->OpenContextMenu( cPosition + m->m_pcView->GetScrollOffset(), nIcon >= 0 );
			if( nIcon < 0 )
				return( os::View::MouseDown( cPosition, nButtons ) );
		}
			
		/* Check if we have a double click */
		if( get_system_time() - m->m_nLastClick < 500000 && m->m_cIcons[nIcon]->m_bSelected && !( GetQualifiers() & os::QUAL_CTRL ) )
		{
			/* Invoke */
			m->Lock();
			m->m_pcControl->Invoked( nIcon, m->m_cIcons[nIcon]->m_pcData );
			m->Unlock();
			
			m->m_bMouseDown = false;
			m->m_bMouseSelectedIcon = true;
			return( os::View::MouseDown( cPosition, nButtons ) );
		}
		
		/* Save values */
		m->m_nLastClick = get_system_time();
		if( nIcon >= 0 )
			m->m_bMouseDownOverIcon = true;
		
		os::View::MouseDown( cPosition, nButtons );
	}
	
	#define DRAG_START_DISTANCE 30
	void MouseMove( const os::Point& cPosition, int nCode, uint32 nButtons, os::Message *pcData )
	{
		
		AutoLocker cLocker( m->m_pcIconLock );		
		if( !m->m_bMouseDown || !( nButtons & 1 ) )	
			return( os::View::MouseMove( cPosition, nCode, nButtons, pcData ) );
			
		
		/* Check what we should do */
		if( !m->m_bMouseDownOverIcon )
		{
			if( !m->m_bSelecting )
				m->m_bSelecting = true;
			/* Draw new frame */
					
			os::Rect cInvalidate( m->m_cLastSelectPosition.x, m->m_cSelectStart.y, cPosition.x, m->m_cLastSelectPosition.y );
			float vTemp;
			if( cInvalidate.left > cInvalidate.right ) {
				vTemp = cInvalidate.right;
				cInvalidate.right = cInvalidate.left;
				cInvalidate.left = vTemp;
			}
			if( cInvalidate.top > cInvalidate.bottom ) {
				vTemp = cInvalidate.bottom;
				cInvalidate.bottom = cInvalidate.top;
				cInvalidate.top = vTemp;
			}
			Invalidate( cInvalidate );
			cInvalidate = os::Rect( m->m_cSelectStart.x, m->m_cLastSelectPosition.y, cPosition.x, cPosition.y );
			if( cInvalidate.left > cInvalidate.right ) {
				vTemp = cInvalidate.right;
				cInvalidate.right = cInvalidate.left;
				cInvalidate.left = vTemp;
			}
			if( cInvalidate.top > cInvalidate.bottom ) {
				vTemp = cInvalidate.bottom;
				cInvalidate.bottom = cInvalidate.top;
				cInvalidate.top = vTemp;
			}
			Invalidate( cInvalidate );
			m->m_cLastSelectPosition = cPosition;

			Flush();
		} else 
		{
			/* Start drag operation */
			if( !m->m_bDragging && ( ( m->m_cMouseDownPos.x - cPosition.x ) * ( m->m_cMouseDownPos.x - cPosition.x )
				  + ( m->m_cMouseDownPos.y - cPosition.y ) * ( m->m_cMouseDownPos.y - cPosition.y ) ) > DRAG_START_DISTANCE )
			{
				m->m_pcControl->DragSelection( cPosition );
				m->m_bDragging = true;
			}
		}
		
		os::View::MouseMove( cPosition, nCode, nButtons, pcData );
	}
	
	void MouseUp( const os::Point& cPosition, uint32 nButtons, os::Message *pcData )
	{
		AutoLocker cLocker( m->m_pcIconLock );		
		/* Handle deselection with pressed ctrl key. We cannot handle it in MouseDown()
		   because we want dragging operations to start with pressed ctrl key */
		int nIcon = 0;
		if( nButtons == 1 && ( GetQualifiers() & os::QUAL_CTRL ) && m->m_bMultiSelect
			&& ( ( nIcon = m->HitTest( cPosition ) ) >= 0 ) && !m->m_bSelecting && !m->m_bDragging ) 
		{
			if( !m->m_bMouseSelectedIcon && m->m_cIcons[nIcon]->m_bSelected ) {
				/* Deselect this icon */
				m->Select( nIcon, false );
				m->m_pcControl->SelectionChanged();
				Flush();
			}
			
		}
		
		/* Handle single click interface */
		if( m->m_bMouseDown && m->m_bMouseDownOverIcon && nButtons == 1 && !m->m_bDragging && !m->m_bSelecting
			&& m->m_bSingleClick && !( GetQualifiers() & ( os::QUAL_CTRL | os::QUAL_SHIFT ) ) )
		{
			int nIcon = m->HitTest( cPosition );
			
			if( nIcon >= 0 )
			{
				/* Invoke */
				m->Lock();
				m->m_pcControl->Invoked( nIcon, m->m_cIcons[nIcon]->m_pcData );
				m->Unlock();
			}
		}
		/* Select the icons in the selection area */
		if( m->m_bSelecting )
		{
			m->Select( m->m_cSelectStart, m->m_cLastSelectPosition, GetQualifiers() & os::QUAL_CTRL );
			m->m_pcControl->SelectionChanged();
			
			Invalidate();
			Flush();		
		}
		
		/* Reset values */
		m->m_bMouseDown = false;
		m->m_bMouseDownOverIcon = false;
		m->m_bDragging = false;
		m->m_bSelecting = false;
		m->m_bMouseSelectedIcon = false;
		
		os::View::MouseUp( cPosition, nButtons, pcData );
	}
	
	void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
	{
		AutoLocker cLocker( m->m_pcIconLock );		
		/* Handle keyboard events */
		switch( pzString[0] )
		{
			case os::VK_UP_ARROW:
				m->SelectNextPrev( false );
				Flush();
			break;
			case os::VK_DOWN_ARROW:
				m->SelectNextPrev( true );
				Flush();
			break;
			case os::VK_RETURN:
			{
				/* Invoke the selected icon */
				m->Lock();
				uint nSelectCount = 0;
				uint nIcon = 0;
				for( uint i = 0; i < m->m_cIcons.size(); i++ )
				{
					if( m->m_cIcons[i]->m_bSelected ) {
						nIcon = i;
						nSelectCount++;
					}
				}
				if( nSelectCount == 1 )
					m->m_pcControl->Invoked( nIcon, m->m_cIcons[nIcon]->m_pcData );	
				m->Unlock();
			}
			break;
		}
		os::View::KeyDown( pzString, pzRawString, nQualifiers );
	}
	
	void FrameSized( const os::Point& cDelta )
	{
		m->LayoutIconsIfNecessary();
	}
	
	void WheelMoved( const os::Point& cDelta )
	{
		if( m->m_pcVScrollBar->IsVisible() )
			m->m_pcVScrollBar->WheelMoved( cDelta );
	}
	
	/* Received when auto scroll is enabled */
	void TimerTick( int nID )
	{
		if( nID != 1 )
			return;
			
		//return;
			
		if( m->m_bScrollDown )
			if( m->m_pcVScrollBar->GetValue().AsInt32() < ( m->m_vLastYPos - GetBounds().Height() ) )
			{
				//std::cout<<"Scroll Down!"<<std::endl;
				float vScroll = GetScrollOffset().y - std::min( 20.0f, m->m_vLastYPos - GetBounds().Height() );
				ScrollTo( os::Point( 0, vScroll ) );
				Flush();
			}
			else
				m->m_bScrollDown = false;
		if( m->m_bScrollUp )
			if( m->m_pcVScrollBar->GetValue().AsInt32() > 0 )
			{
				//std::cout<<"Scroll Up!"<<std::endl;
				float vScroll = GetScrollOffset().y + std::min( 20.0f, m->m_pcVScrollBar->GetValue().AsFloat() );
				ScrollTo( os::Point( 0, vScroll ) );
				Flush();
			}
			else
				m->m_bScrollUp = false;
	}

private:
	IconView::Private* m;
};

/** Constructor.
 * \par Description:
 * IconView constructor. 
 * \param cFrame - Frame of the iconview.
 * \param zName - Name of the iconview.
 * \param nResizeMask - resize mask.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
IconView::IconView( const Rect& cFrame, os::String zName, uint32 nResizeMask )
				: os::Control( cFrame, zName, "", NULL, nResizeMask )
{
	m = new IconView::Private( this );
	
	
	/* Create scrollbars */
	m->m_pcVScrollBar = new os::ScrollBar( os::Rect(), "filesystem_scroll", NULL, 0, 0, os::VERTICAL,
									os::CF_FOLLOW_TOP | os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_RIGHT );
	m->m_pcVScrollBar->SetSteps( 50, 100 );
	os::Rect cNewFrame = GetBounds();
	cNewFrame.left = cNewFrame.right - m->m_pcVScrollBar->GetPreferredSize( false ).x;
	m->m_pcVScrollBar->SetFrame( cNewFrame );
	AddChild( m->m_pcVScrollBar );
	
	m->m_pcHScrollBar = new os::ScrollBar( os::Rect(), "filesystem_scroll", NULL, 0, 0, os::HORIZONTAL,
									os::CF_FOLLOW_LEFT | os::CF_FOLLOW_BOTTOM | os::CF_FOLLOW_RIGHT );
	m->m_pcHScrollBar->SetSteps( 50, 100 );
	cNewFrame = GetBounds();
	cNewFrame.top = cNewFrame.bottom - m->m_pcHScrollBar->GetPreferredSize( false ).y;
	m->m_pcHScrollBar->SetFrame( cNewFrame );
	AddChild( m->m_pcHScrollBar );
		
	/* Create main view */	
	cNewFrame = GetBounds();
	cNewFrame.right -= m->m_pcVScrollBar->GetPreferredSize( false ).x + 1;
	cNewFrame.bottom -= m->m_pcHScrollBar->GetPreferredSize( false ).y + 1;
	m->m_vScrollBarWidth = m->m_pcVScrollBar->GetPreferredSize( false ).x + 1;
	m->m_vScrollBarHeight = m->m_pcHScrollBar->GetPreferredSize( false ).y + 1;
	m->m_pcView = new MainView( cNewFrame, m );
	AddChild( m->m_pcView );
	
	m->m_pcHScrollBar->SetScrollTarget( m->m_pcView );
	m->m_pcVScrollBar->SetScrollTarget( m->m_pcView );
	
	Clear();
}

IconView::~IconView()
{
	Clear();
	RemoveChild( m->m_pcView );
	delete( m->m_pcView );
	RemoveChild( m->m_pcVScrollBar );
	RemoveChild( m->m_pcHScrollBar );
	if( m->m_pcBackground )
		delete( m->m_pcBackground );
	delete( m->m_pcVScrollBar );
	delete( m->m_pcHScrollBar );
	delete( m->m_pcSelChangeMsg );
	delete( m->m_pcIconLock );
	
	delete( m );
}

/** Returns the curent view type.
 * \par Description:
 * \return The current view type as VIEW_* enums.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
IconView::view_type IconView::GetView() const
{
	return( m->m_eType );
}


/** Sets a new view type.
 * \par Description:
 * Sets a new view type and updates the view.
 * \param eType - View type.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetView( view_type eType )
{
	m->m_eType = eType;
	Layout();
}


/** Sets a new background image.
 * \par Description:
 * Sets a new background image and updates the view.
 * \par Note:
 * The previous image will be deleted.
 * \param pcImage - The new image.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetBackground( os::Image* pcImage )
{
	if( m->m_pcBackground )
		delete( m->m_pcBackground );
	
	m->m_pcBackground = pcImage;
	m->m_pcView->Invalidate();
	m->m_pcView->Flush();
}


/** Enables/Disables single click mode.
 * \par Description:
 * Enables/Disables single click mode. In single click mode the icons will
 * already be invoked with a single click.
 * \param bSingleClick - Whether single click mode is enabled.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetSingleClick( bool bSingleClick )
{
	m->m_bSingleClick = bSingleClick;
}


/** Returns whether single click mode is enabled.
 * \par Description:
 * Returns whether single click mode is enabled.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
bool IconView::IsSingleClick() const
{
	return( m->m_bSingleClick );
}

/** Enables/Disables sorting.
 * \par Description:
 * Enables/Disables the sorting of the icons.
 * \param bAutoSort - Whether sorting is enabled.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetAutoSort( bool bAutoSort )
{
	m->m_bAutoSort = bAutoSort;
}

/** Returns whether sorting is enabled.
 * \par Description:
 * Returns whether sorting is enabled.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
bool IconView::IsAutoSort() const
{
	return( m->m_bAutoSort );
}

/** Enables/Disables multiselect.
 * \par Description:
 * Enables/Disables whether the selection of multiple icons is allowed.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetMultiSelect( bool bMultiSelect )
{
	m->m_bMultiSelect = bMultiSelect;
}

/** Returns whether multiselect is enabled.
 * \par Description:
 * Returns whether multiselect is enabled.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
bool IconView::IsMultiSelect() const
{
	return( m->m_bMultiSelect );
}

/** Sets a new background color.
 * \par Description:
 * Sets a new background color and updates the view.
 * \par Note:
 * Not used if a background image is set.
 * \param sColor - The new color.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetBackgroundColor( os::Color32_s sColor )
{
	m->m_sBackgroundColor = sColor;
	m->m_pcView->Invalidate();
	m->m_pcView->Flush();
}


/** Sets a new text color.
 * \par Description:
 * Sets a new text color and updates the view.
 * \par Note: SetTextColor overwrites SetTextShadowColor to disable shadows
 * Please make sure that the background color/image doesn’t make the text
 * invisible.
 * \param sColor - The new color.
 * \author	Arno Klenke (arno_klenke@yahoo.de) Andreas Benzler (andreas.benzler@t-online.de)
 *****************************************************************************/
void IconView::SetTextColor( os::Color32_s sColor )
{
	m->m_sTextColor = sColor;
	m->m_sTextShadowColor = sColor;
		
	m->m_pcView->Invalidate();
	m->m_pcView->Flush();
}


/** Sets a new color for the shadows of the text.
 * \par Description:
 * Sets a new color for the shadows of the text.
 * \par Note: SetTextColor overwrites SetTextShadowColor to disable shadows.
 * \param sColor - The new color.
 * \author	Arno Klenke (arno_klenke@yahoo.de) Andreas Benzler (andreas.benzler@t-online.de)
 *****************************************************************************/
void IconView::SetTextShadowColor( os::Color32_s sColor )
{
	m->m_sTextShadowColor = sColor;
	m->m_pcView->Invalidate();
	m->m_pcView->Flush();
}

/** Sets a new color for the selection.
 * \par Description:
 * Sets a new color for the selection rectangle.
 * \par Note:
 * Please make sure that the color doesn’t make the text
 * invisible.
 * \param sColor - The new color.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetSelectionColor( os::Color32_s sColor )
{
	m->m_sSelectionColor = sColor;
	m->m_pcView->Invalidate();
	m->m_pcView->Flush();
}


/** Clears the iconview.
 * \par Description:
 * Clears the iconview and deletes all icons.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::Clear()
{
	m->Lock();
	for( uint i = 0; i < m->m_cIcons.size(); i++ )
	{
		delete( m->m_cIcons[i]->m_pcImage );
		delete( m->m_cIcons[i]->m_pcData );
		delete( m->m_cIcons[i] );
	}
	m->m_cIcons.clear();
	m->Unlock();
	m->LayoutIcons();
}


/** Adds a new icon to the iconview.
 * \par Description:
 * Adds a new icon to the iconview.
 * \par Note:
 * You need to call Layout() to make the new icons visible!
 * \param pcImage - The icon.
 * \param pcData - Private data of the icon. A subclass of os::IconData can be used.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
uint IconView::AddIcon( os::Image* pcImage, os::IconData* pcData )
{
	if( pcImage == NULL )
		return( 0 );
	//std::cout<<"Add "<<zName.c_str()<<std::endl;
	Icon* psIcon = new Icon;
	psIcon->m_cPosition = os::Point( 0, 0 );
	psIcon->m_zStrings.clear();
	psIcon->m_pcImage = pcImage;
	psIcon->m_pcData = pcData;
	psIcon->m_bSelected = false;
	psIcon->m_bLayouted = false;
	m->Lock();
	m->m_cIcons.push_back( psIcon );
	m->Unlock();
	return( m->m_cIcons.size() - 1 );
}


/** Removes an icon.
 * \par Description:
 * Removes an icon from the iconview.
 * \par Note:
 * You need to call Layout() to make the changes visible!
 * \param nIcon - Index of the icon.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::RemoveIcon( uint nIcon )
{
	if( nIcon >= m->m_cIcons.size() )
		return;
	m->Lock();
	delete( m->m_cIcons[nIcon]->m_pcImage );
	delete( m->m_cIcons[nIcon]->m_pcData );
	delete( m->m_cIcons[nIcon] );
	m->m_cIcons.erase( m->m_cIcons.begin() + nIcon );
	m->Unlock();
}

/** Returns the number of icons.
 * \par Description:
 * Returns the number of icons of the iconview.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
uint IconView::GetIconCount()
{
	return( m->m_cIcons.size() );
}


/** Returns a string of an icon.
 * \par Description:
 * Returns a string of an icon of the iconview.
 * \param nIcon - Index of the icon.
 * \param nString - Index of the string.
 * \return The string or "" if one of the index is invalid.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String IconView::GetIconString( uint nIcon, uint nString )
{
	if( nIcon >= m->m_cIcons.size() || nString >= m->m_cIcons[nIcon]->m_zStrings.size() )
		return( "" );
	return( m->m_cIcons[nIcon]->m_zStrings[nString] );
}


/** Returns the image of an icon.
 * \par Description:
 * Returns the image of an icon of the iconview.
 * \param nIcon - Index of the icon.
 * \return The image or NULL.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::Image* IconView::GetIconImage( uint nIcon )
{
	if( nIcon >= m->m_cIcons.size() )
		return( NULL );
	else
		return( m->m_cIcons[nIcon]->m_pcImage );
}

/** Returns the private data of an icon.
 * \par Description:
 * Returns the private data of an icon of the iconview.
 * \param nIcon - Index of the icon.
 * \return The data or NULL.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::IconData* IconView::GetIconData( uint nIcon )
{
	if( nIcon >= m->m_cIcons.size() )
		return( NULL );
	else
		return( m->m_cIcons[nIcon]->m_pcData );
}

/** Returns whether one icon is selected.
 * \par Description:
 * Returns whether one icon of the iconview is selected.
 * \param nIcon - Index of the icon.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
bool IconView::GetIconSelected( uint nIcon )
{
	if( nIcon >= m->m_cIcons.size() )
		return( false );
	else
		return( m->m_cIcons[nIcon]->m_bSelected );
}


/** Returns the current position of an icon.
 * \par Description:
 * RReturns the current position of an icon of the iconview.
 * \param nIcon - Index of the icon.
 * \return The position.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::Point IconView::GetIconPosition( uint nIcon )
{
	if( nIcon >= m->m_cIcons.size() )
		return( os::Point( 0, 0 ) );
	else
		return( m->m_cIcons[nIcon]->m_cPosition );
}

/** Adds a string of an icon.
 * \par Description:
 * Add a a string of an icon.
 * \par Note:
 * Only the first string will be displayed in all view types. The additional ones
 * only in VIEW_DETAILS.
 * \param nIcon - Index of the icon.
 * \param zString - String.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::AddIconString( uint nIcon, os::String zString )
{
	if( nIcon >= m->m_cIcons.size() )
		return;
		
	m->Lock();
	m->m_cIcons[nIcon]->m_zStrings.push_back( zString );
	m->Unlock();
}

/** Sets a string of an icon.
 * \par Description:
 * Sets a a string of an icon.
 * \par Note:
 * You have to add new strings using the AddIconString() method.
 * \param nIcon - Index of the icon.
 * \param nString - Index of the string.
 * \param zString - String.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetIconString( uint nIcon, uint nString, os::String zString )
{
	if( nIcon >= m->m_cIcons.size() || nString >= m->m_cIcons[nIcon]->m_zStrings.size() )
		return;
	
	m->Lock();
	m->m_cIcons[nIcon]->m_zStrings[nString] = zString;
	
	m->m_pcView->Invalidate( os::Rect( m->m_cIcons[nIcon]->m_cPosition - os::Point( 3, 3 ), m->m_cIcons[nIcon]->m_cPosition
											+ GetIconSize() + os::Point( 3, 3 ) ) );
	m->Unlock();
	m->m_pcView->Flush();
}

/** Sets the image of an icon.
 * \par Description:
 * Sets the image of an icon.
 * \param nIcon - Index of the icon.
 * \param pcImage - New image. The old one will be deleted.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetIconImage( uint nIcon, os::Image* pcImage )
{
	if( nIcon >= m->m_cIcons.size() )
		return;
		
	m->Lock();
	
	delete( m->m_cIcons[nIcon]->m_pcImage );
	m->m_cIcons[nIcon]->m_pcImage = pcImage;
	
	m->m_pcView->Invalidate( os::Rect( m->m_cIcons[nIcon]->m_cPosition - os::Point( 3, 3 ), m->m_cIcons[nIcon]->m_cPosition
											+ GetIconSize() + os::Point( 3, 3 ) ) );
	m->Unlock();
	m->m_pcView->Flush();
}

/** Sets the selection status of an icon.
 * \par Description:
 * Sets the selection status of an icon and updates the view.
 * \param nIcon - Index of the icon.
 * \param bSelected - New select status.
 * \param bDeselectAll - Whether all icons should be deslected first.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetIconSelected( uint nIcon, bool bSelected, bool bDeselectAll )
{
	if( nIcon >= m->m_cIcons.size() )
		return;
	m->Lock();
	if( bDeselectAll ) {
		for( uint i = 0; i < m->m_cIcons.size(); i++ )
			if( i != nIcon )
			{
				m->Select( i, false );
			}
	}
	m->Select( nIcon, bSelected );
	m->m_pcView->Flush();
	m->Unlock();
	SelectionChanged();
}


/** Sets the position of an icon.
 * \par Description:
 * Sets the position of an icon.
 * \param nIcon - Index of the icon.
 * \param cPosition - the new position.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetIconPosition( uint nIcon, os::Point cPosition )
{
	m->Lock();
	if( nIcon < m->m_cIcons.size() ) {
		m->m_pcView->Invalidate( os::Rect( m->m_cIcons[nIcon]->m_cPosition - os::Point( 3, 3 ), m->m_cIcons[nIcon]->m_cPosition
											+ GetIconSize() + os::Point( 3, 3 ) ) );
		m->m_cIcons[nIcon]->m_cPosition = cPosition;
		m->m_pcView->Invalidate( os::Rect( m->m_cIcons[nIcon]->m_cPosition - os::Point( 3, 3 ), m->m_cIcons[nIcon]->m_cPosition
											+ GetIconSize() + os::Point( 3, 3 ) ) );
		m->m_pcView->Flush();											
	}
	m->Unlock();
}

/** Returns the current icon size that is assumed by the iconview.
 * \par Description:
 * Returns the current icon size that is assumed by the iconview.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::Point IconView::GetIconSize()
{
	return( os::Point( m->m_vIconWidth, m->m_vIconHeight ) );
}


/** Renders an icon in a view.
 * \par Description:
 * Renders an icon in a view. Should only be used in special cases.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::RenderIcon( uint nIcon, os::View* pcView, os::Point cPosition )
{
	m->RenderIcon( nIcon, pcView, cPosition );
}

/** Renders an icon in a view.
 * \par Description:
 * Renders an icon in a view. Should only be used in special cases.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::RenderIcon( os::String zName, os::Image* pcImage, os::View* pcView, os::Point cPosition )
{
	m->RenderIcon( zName, pcImage, pcView, cPosition );
}

/** Relayouts the view.
 * \par Description:
 * Relayouts and updates the view. Necessary after calls like AddIcon() or RemoveIcon().
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::Layout()
{
	//std::cout<<"Layout!"<<std::endl;
	m->SortIcons();
	m->LayoutIcons();
}

os::Point IconView::ConvertToView( os::Point cPoint )
{
	return( cPoint - m->m_pcView->GetScrollOffset() );
	
}

/** Start scrolling.
 * \par Description:
 * Start scrolling.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::StartScroll( scroll_direction eDirection )
{
	if( eDirection == SCROLL_DOWN )
	{
		if( !m->m_bScrollDown )
		{
			m->m_bScrollDown = true;
			if( m->m_bScrollUp == false )
			{
				os::Looper *pcLooper = GetLooper();

				if( pcLooper != NULL )
				{
					pcLooper->AddTimer( m->m_pcView, 1, 200000, false );
				}
			}
			else
			{
				m->m_bScrollUp = false;
			}
		}
	}
	if( eDirection == SCROLL_UP )
	{
		if( !m->m_bScrollUp )
		{
			m->m_bScrollUp = true;
			if( m->m_bScrollDown == false )
			{
				os::Looper *pcLooper = GetLooper();

				if( pcLooper != NULL )
				{
					pcLooper->AddTimer( m->m_pcView, 1, 200000, false );
				}
			}
			else
			{
				m->m_bScrollDown = false;
			}
		}
	}
}

/** Stops scrolling.
 * \par Description:
 * Stops scrolling.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::StopScroll()
{
	if( m->m_bScrollDown || m->m_bScrollUp )
	{
		m->m_bScrollDown = m->m_bScrollUp = false;
		os::Looper *pcLooper = GetLooper();

		if( pcLooper != NULL )
		{
			pcLooper->RemoveTimer( m->m_pcView, 1 );
		}
	}
}

/** Scrolls to an icon.
 * \par Description:
 * Scrolls the view to make one icon visible.
 * \param nIcon - Icon
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::ScrollToIcon( uint nIcon )
{
	m->Lock();
	if( nIcon < m->m_cIcons.size() ) {
		os::Rect cFrame = m->m_pcView->GetBounds();
		os::Rect cIconFrame( m->m_cIcons[nIcon]->m_cPosition - os::Point( 3, 3 ), m->m_cIcons[nIcon]->m_cPosition +
																os::Point( m->m_vIconWidth, m->m_vIconHeight ) + os::Point( 3, 3 ) );
		if( !( cFrame.DoIntersect( os::Point( 0, cIconFrame.top ) ) && cFrame.DoIntersect( os::Point( 0, cIconFrame.bottom ) ) ) && !( m->m_bScrollDown || m->m_bScrollUp ) )
		{
			float vScroll = -( cIconFrame.top - m->m_pcView->GetBounds().Height() / 2 + cIconFrame.Height() / 2 );
			if( vScroll > 0 )
				vScroll = 0;
			if( vScroll < -( m->m_vLastYPos - cFrame.Height() ) )
				vScroll = -( m->m_vLastYPos - cFrame.Height() );
			m->m_pcView->ScrollTo( os::Point( 0, vScroll ) );
		}
	}
	m->Unlock();
}


void IconView::EnableStatusChanged( bool bIsEnabled )
{
}


/** Sets the selection change message.
 * \par Description:
 * Sets the message that is sent when the selection changes.
 * \param pcMessage - New message.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetSelChangeMsg( os::Message* pcMessage )
{
	if( m->m_pcSelChangeMsg != pcMessage )
	{
		delete( m->m_pcSelChangeMsg );
		m->m_pcSelChangeMsg = pcMessage;
	}
}

/** Sets the invoke message.
 * \par Description:
 * Sets the message that is sent when an icon is invoked.
 * \param pcMessage - New message.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SetInvokeMsg( os::Message* pcMessage )
{
	SetMessage( pcMessage );
}


/** Returns the selection change message.
 * \par Description:
 * Returns the message that is sent when the selection changes.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::Message* IconView::GetSelChangeMsg()
{
	return( m->m_pcSelChangeMsg );
}


/** Returns the invoke message.
 * \par Description:
 * Returns the message that is sent when an icon is invoked.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::Message* IconView::GetInvokeMsg()
{
	return( GetMessage() );
}

/** Called when an icon is invoked.
 * \par Description:
 * Called when an icon is invoked. The default action is to send the message
 * set by SetInvokeMsg()
 * \param nIcon - Index of the invoked icon.
 * \param pcData - Data of the invoked icon.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::Invoked( uint nIcon, os::IconData* pcData )
{
	Invoke( NULL );
}


/** Called when the selection changes.
 * \par Description:
 * Called when an icon is selected. The default action is to send the message
 * set by SetSelChangeMsg()
 * \param nIcon - Index of the invoked icon.
 * \param pcData - Data of the invoked icon.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::SelectionChanged()
{
	if( m->m_pcSelChangeMsg != NULL )
	{
		Invoke( m->m_pcSelChangeMsg );
	}
}


/** Called when an icon should be dragged.
 * \par Description:
 * Called when an icon should be dragged.
 * \param cStartPoint - Start position of the drag.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::DragSelection( os::Point cStartPoint )
{
}


/** Called when the context menu of one icon should be opened.
 * \par Description:
 * Called when the context menu of one icon should be opened.
 * \param cPosition - Position.
 * \param bMouseOverIcon - Whether the contextmenu of an icon should be opened.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconView::OpenContextMenu( os::Point cPosition, bool bMouseOverIcon )
{
}

void IconView::__ICV_reserved1__() {}
void IconView::__ICV_reserved2__() {}
void IconView::__ICV_reserved3__() {}
void IconView::__ICV_reserved4__() {}
void IconView::__ICV_reserved5__() {}

