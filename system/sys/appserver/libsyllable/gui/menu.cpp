
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

#include <stdio.h>
#include <assert.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <gui/menu.h>
#include <gui/font.h>
#include <gui/bitmap.h>
#include <util/message.h>
#include <atheos/kernel.h>

namespace os
{
	class MenuWindow:public Window
	{
	      public:
		MenuWindow( const Rect & cFrame, Menu * pcMenu );
		 ~MenuWindow();

		void ShutDown()
		{
			if( m_pcMenu != NULL )
			{
				RemoveChild( m_pcMenu );
				m_pcMenu = NULL;
			}
			PostMessage( M_TERMINATE );
		}
	      private:
		  Menu * m_pcMenu;
	};

}

using namespace os;

Bitmap *MenuItem::s_pcSubMenuBitmap = NULL;

#define SUB_MENU_ARROW_W 10
#define SUB_MENU_ARROW_H 10

uint8 nSubMenuArrowData[] = {
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};

Locker Menu::s_cMutex( "menu_mutex" );

static Color32_s Tint( const Color32_s & sColor, float vTint )
{
	int r = int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int g = int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int b = int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) ) );

	if( r < 0 )
		r = 0;
	else if( r > 255 )
		r = 255;
	if( g < 0 )
		g = 0;
	else if( g > 255 )
		g = 255;
	if( b < 0 )
		b = 0;
	else if( b > 255 )
		b = 255;
	return ( Color32_s( r, g, b, sColor.alpha ) );
}

class Menu::Private
{
      public:
	Private():m_cMutex( "menu_mutex" )
	{
	}
	std::string m_cTitle;
	Message m_cCloseMsg;
	Messenger m_cCloseMsgTarget;
	Locker m_cMutex;

};

class MenuItem::Private
{
      public:
	Private()
	{
		m_pcNext = NULL;
		m_pcSuperMenu = NULL;
		m_pcSubMenu = NULL;
		m_bIsHighlighted = false;
		m_bIsEnabled = true;
		m_pzLabel = NULL;
	}
	~Private()
	{
		if( m_pzLabel )
			delete[]m_pzLabel;
		if( m_pcSubMenu )
			delete m_pcSubMenu;
	}

	MenuItem *m_pcNext;
	Menu *m_pcSuperMenu;
	Menu *m_pcSubMenu;
	Rect m_cFrame;

	char *m_pzLabel;

	bool m_bIsHighlighted;
	bool m_bIsEnabled;
};


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem::MenuItem( const char *pzLabel, Message * pcMsg ):Invoker( pcMsg, NULL, NULL )
{
	m = new Private;

	if( NULL != pzLabel )
	{
		m->m_pzLabel = new char[strlen( pzLabel ) + 1];

		strcpy( m->m_pzLabel, pzLabel );
	}

	if( s_pcSubMenuBitmap == NULL )
	{
		Rect cSubMenuBitmapRect;

		cSubMenuBitmapRect.left = 0;
		cSubMenuBitmapRect.top = 0;
		cSubMenuBitmapRect.right = SUB_MENU_ARROW_W;
		cSubMenuBitmapRect.bottom = SUB_MENU_ARROW_H;

		s_pcSubMenuBitmap = new Bitmap( cSubMenuBitmapRect.Width(), cSubMenuBitmapRect.Height(  ), CS_RGBA32, Bitmap::SHARE_FRAMEBUFFER );
		memcpy( s_pcSubMenuBitmap->LockRaster(), nSubMenuArrowData, ( unsigned int )( cSubMenuBitmapRect.Width(  ) * cSubMenuBitmapRect.Height(  ) * 4 ) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem::MenuItem( Menu * pcMenu, Message * pcMsg ):Invoker( pcMsg, NULL, NULL )
{
	m = new Private;
	m->m_pcSubMenu = pcMenu;
	pcMenu->m_pcSuperItem = this;

	std::string cLabel = pcMenu->GetLabel();

	if( cLabel.empty() == false )
	{
		m->m_pzLabel = new char[cLabel.size() + 1];

		strcpy( m->m_pzLabel, cLabel.c_str() );
	}
	else
	{
		m->m_pzLabel = NULL;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem::~MenuItem()
{
	delete m;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool MenuItem::Invoked( Message * pcMessage )
{
	return ( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu *MenuItem::GetSubMenu() const
{
	return ( m->m_pcSubMenu );
}

void MenuItem::SetSubMenu( Menu * pcMenu )
{
	m->m_pcSubMenu = pcMenu;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu *MenuItem::GetSuperMenu() const
{
	return ( m->m_pcSuperMenu );
}

void MenuItem::SetSuperMenu( Menu * pcMenu )
{
	m->m_pcSuperMenu = pcMenu;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Rect MenuItem::GetFrame() const
{
	return ( m->m_cFrame );
}

void MenuItem::SetFrame( const Rect & cFrame )
{
	m->m_cFrame = cFrame;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point MenuItem::GetContentSize()
{
	Menu *pcMenu = GetSuperMenu();

	// Are we a super-menu?  If so, we need to make room for the sub-menu arrow
	if( m->m_pzLabel != NULL && pcMenu != NULL && GetSubMenu() != NULL && m->m_pcSuperMenu->GetLayout(  ) == ITEMS_IN_COLUMN )
	{
		font_height sHeight;

		pcMenu->GetFontHeight( &sHeight );

		return ( Point( pcMenu->GetStringWidth( GetLabel() ) + 32, sHeight.ascender + sHeight.descender + sHeight.line_gap ) );
	}

	// Are we a "normal" menu, in a column of other menus?  If so, we need to allow for a gap on the left
	if( m->m_pzLabel != NULL && pcMenu != NULL && m->m_pcSuperMenu->GetLayout() != ITEMS_IN_ROW )
	{
		font_height sHeight;

		pcMenu->GetFontHeight( &sHeight );

		return ( Point( pcMenu->GetStringWidth( GetLabel() ) + 16, sHeight.ascender + sHeight.descender + sHeight.line_gap ) );

	}

	// We are a menu which is in a row of menus, I.e. a menu bar.  We don't need a gap, and we can't have a sub-menu
	if( m->m_pzLabel != NULL && pcMenu != NULL )
	{
		font_height sHeight;

		pcMenu->GetFontHeight( &sHeight );

		return ( Point( pcMenu->GetStringWidth( GetLabel() ) + 4, sHeight.ascender + sHeight.descender + sHeight.line_gap ) );

	}

	// We don't have a label, probably
	return ( Point( 0.0f, 0.0f ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

const char *MenuItem::GetLabel() const
{
	return ( m->m_pzLabel );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point MenuItem::GetContentLocation() const
{
	return ( Point( m->m_cFrame.left, m->m_cFrame.top ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MenuItem::Draw()
{
	Menu *pcMenu = GetSuperMenu();

	if( pcMenu == NULL )
	{
		return;
	}

	const char *pzLabel = GetLabel();

	if( pzLabel == NULL )
	{
		return;
	}

	Rect cFrame = GetFrame();

	font_height sHeight;

	pcMenu->GetFontHeight( &sHeight );

	if( m->m_bIsEnabled )
	{
		if( m->m_bIsHighlighted )
		{
			pcMenu->SetFgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
		}
		else
		{
			pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
		}

		pcMenu->FillRect( GetFrame() );

		if( m->m_bIsHighlighted )
		{
			pcMenu->SetFgColor( get_default_color( COL_SEL_MENU_TEXT ) );
			pcMenu->SetBgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
		}
		else
		{
			pcMenu->SetFgColor( get_default_color( COL_MENU_TEXT ) );
			pcMenu->SetBgColor( get_default_color( COL_MENU_BACKGROUND ) );
		}

	}
	else
	{
		pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
		pcMenu->FillRect( GetFrame() );
		pcMenu->SetFgColor( 255, 255, 255 );
		pcMenu->SetBgColor( get_default_color( COL_MENU_BACKGROUND ) );
	}

	float vCharHeight = sHeight.ascender + sHeight.descender + sHeight.line_gap;
	float y = cFrame.top + ( cFrame.Height() + 1.0f ) / 2 - vCharHeight / 2 + sHeight.ascender + sHeight.line_gap * 0.5f;
	float x = cFrame.left + ( ( m->m_pcSuperMenu->GetLayout() == ITEMS_IN_COLUMN ) ? 16 : 2 );

	pcMenu->MovePenTo( x, y );
	pcMenu->DrawString( pzLabel );

	if( IsEnabled() == false )
	{
		pcMenu->SetFgColor( 100, 100, 100 );
		pcMenu->MovePenTo( x - 1.0f, y - 1.0f );
		pcMenu->SetDrawingMode( DM_OVER );
		pcMenu->DrawString( GetLabel() );
		pcMenu->SetDrawingMode( DM_COPY );
	}

	// If we are the super-menu, draw the sub-menu triangle on the right of the label
	if( GetSubMenu() != NULL && m->m_pcSuperMenu->GetLayout(  ) == ITEMS_IN_COLUMN )
	{
		Rect cSourceRect = s_pcSubMenuBitmap->GetBounds();
		Rect cTargetRect;

		cTargetRect = cSourceRect.Bounds() + Point( m->m_cFrame.right - SUB_MENU_ARROW_W, m->m_cFrame.top + ( m->m_cFrame.Height(  ) / 2 ) - 4.0f );

		pcMenu->SetDrawingMode( DM_OVER );
		pcMenu->DrawBitmap( s_pcSubMenuBitmap, cSourceRect, cTargetRect );
		pcMenu->SetDrawingMode( DM_COPY );
	}

}

void MenuItem::SetEnable( bool bEnabled )
{
	m->m_bIsEnabled = bEnabled;
	Draw();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void MenuItem::DrawContent()
{
}

void MenuItem::SetHighlighted( bool bHighlighted )
{
	m->m_bIsHighlighted = bHighlighted;
	Draw();
}

bool MenuItem::IsHighlighted() const
{
	return m->m_bIsHighlighted;
}

bool MenuItem::IsEnabled() const
{
	return m->m_bIsEnabled;
}

MenuItem *MenuItem::GetNext()
{
	return m->m_pcNext;
}

void MenuItem::SetNext( MenuItem * pcItem )
{
	m->m_pcNext = pcItem;
}




MenuSeparator::MenuSeparator():MenuItem( static_cast < char *>( NULL ), NULL )
{
}

MenuSeparator::~MenuSeparator()
{
}

Point MenuSeparator::GetContentSize()
{
	Menu *pcMenu = GetSuperMenu();

	if( pcMenu != NULL )
	{
		font_height sHeight;

		pcMenu->GetFontHeight( &sHeight );

		if( pcMenu->GetLayout() == ITEMS_IN_ROW )
		{
			return ( Point( 6.0f, 0.0f ) );
		}
		else
		{
			return ( Point( 0.0f, 6.0f ) );
		}
	}
	return ( Point( 0.0f, 0.0f ) );
}

void MenuSeparator::Draw()
{
	Menu *pcMenu = GetSuperMenu();

	if( pcMenu == NULL )
	{
		return;
	}
	Rect cFrame = GetFrame();
	Rect cMFrame = pcMenu->GetBounds();

	if( pcMenu->GetLayout() == ITEMS_IN_ROW )
	{
		cFrame.top = cMFrame.top;
		cFrame.bottom = cMFrame.bottom;
	}
	else
	{
		cFrame.left = cMFrame.left;
		cFrame.right = cMFrame.right;
	}
	pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
	pcMenu->FillRect( cFrame );

	if( pcMenu->GetLayout() == ITEMS_IN_ROW )
	{
		float x = floor( cFrame.left + ( cFrame.Width() + 1.0f ) * 0.5f );

		pcMenu->SetFgColor( get_default_color( COL_SHADOW ) );
		pcMenu->DrawLine( Point( x, cFrame.top + 2.0f ), Point( x, cFrame.bottom - 2.0f ) );
		x += 1.0f;
		pcMenu->SetFgColor( get_default_color( COL_SHINE ) );
		pcMenu->DrawLine( Point( x, cFrame.top + 2.0f ), Point( x, cFrame.bottom - 2.0f ) );
	}
	else
	{
		float y = floor( cFrame.top + ( cFrame.Height() + 1.0f ) * 0.5f );

		pcMenu->SetFgColor( get_default_color( COL_SHADOW ) );
		pcMenu->DrawLine( Point( cFrame.left + 4.0f, y ), Point( cFrame.right - 4.0f, y ) );
		y += 1.0f;
		pcMenu->SetFgColor( get_default_color( COL_SHINE ) );
		pcMenu->DrawLine( Point( cFrame.left + 4.0f, y ), Point( cFrame.right - 4.0f, y ) );
	}

}

void MenuSeparator::DrawContent()
{
}

void MenuSeparator::SetHighlighted( bool bHighlighted )
{
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuWindow::MenuWindow( const Rect & cFrame, Menu * pcMenu ):Window( cFrame, "menu", "menu", WND_NO_BORDER | WND_SYSTEM )
{
	// If the machine is very loaded the previous window hosting the menu
	// might still own it, so we must wait til it terminates.
	while( pcMenu->GetWindow() != NULL )
	{
		snooze( 50000 );
	}
	m_pcMenu = pcMenu;
	AddChild( pcMenu );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuWindow::~MenuWindow()
{
	if( m_pcMenu != NULL )
	{
		RemoveChild( m_pcMenu );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu::Menu( Rect cFrame, const char *pzTitle, MenuLayout_e eLayout, uint32 nResizeMask, uint32 nFlags ):View( cFrame, "_menu_", nResizeMask, nFlags & ~WID_FULL_UPDATE_ON_RESIZE )
{
	m = new Private;
	m_bIsTracking = false;
	m_cItemBorders = Rect( 3, 2, 3, 2 );
	m_eLayout = eLayout;
	m_pcWindow = NULL;
	m_pcFirstItem = NULL;
	m_nItemCount = 0;
	m_pcSuperItem = NULL;
	m_bHasOpenChilds = false;
	m_bCloseOnMouseUp = false;
	m_hTrackPort = -1;
	m_bEnabled = true;

	if( NULL != pzTitle )
	{
		m->m_cTitle = pzTitle;
	}
	else
	{
	}
	m_pcRoot = this;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu::~Menu()
{
	while( m_pcFirstItem != NULL )
	{
		MenuItem *pcItem = m_pcFirstItem;

		m_pcFirstItem = pcItem->GetNext();
		delete pcItem;
	}
	delete m;
}

int Menu::Lock() const
{
	return ( m_pcRoot->m->m_cMutex.Lock() );
}

void Menu::Unlock() const
{
	m_pcRoot->m->m_cMutex.Unlock();
}

std::string Menu::GetLabel() const
{
	return ( m->m_cTitle );
}

MenuLayout_e Menu::GetLayout() const
{
	return ( m_eLayout );
}

void Menu::FrameSized( const Point & cDelta )
{
	if( cDelta.x > 0.0f )
	{
		Rect cBounds = GetBounds();

		cBounds.left = cBounds.right - cDelta.x;
		Invalidate( cBounds );
		Flush();
	}
	else if( cDelta.x < 0.0f )
	{
		Rect cBounds = GetBounds();

		cBounds.left = cBounds.right - 2.0f;
		Invalidate( cBounds );
		Flush();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::SetRoot( Menu * pcRoot )
{
	m_pcRoot = pcRoot;

	MenuItem *pcTmp;

	for( pcTmp = m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->GetNext() )
	{
		if( pcTmp->GetSubMenu() != NULL )
		{
			pcTmp->GetSubMenu()->SetRoot( pcRoot );
		}
	}

}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::StartOpenTimer( bigtime_t nDelay )
{
	Looper *pcLooper = GetLooper();

	if( pcLooper != NULL )
	{
		pcLooper->Lock();
		pcLooper->AddTimer( this, 123, nDelay, true );
		pcLooper->Unlock();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::OpenSelection()
{
	MenuItem *pcItem = FindMarked();

	if( pcItem == NULL )
	{
		return;
	}

	MenuItem *pcTmp;

	for( pcTmp = m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->GetNext() )
	{
		if( pcTmp == pcItem )
		{
			continue;
		}
		if( pcTmp->GetSubMenu() != NULL && pcTmp->GetSubMenu(  )->m_pcWindow != NULL )
		{
			pcTmp->GetSubMenu()->Close( true, false );
			break;
		}
	}

	if( pcItem->GetSubMenu() == NULL || pcItem->GetSubMenu(  )->m_pcWindow != NULL )
	{
		return;
	}

	Rect cFrame = pcItem->GetFrame();
	Point cScrPos;

	Point cScreenRes( Desktop().GetResolution(  ) );

	Point cLeftTop( 0, 0 );

	ConvertToScreen( &cLeftTop );

	if( m_eLayout == ITEMS_IN_ROW )
	{
		cScrPos.x = cLeftTop.x + cFrame.left - m_cItemBorders.left;

		float nHeight = pcItem->GetSubMenu()->m_cSize.y;

		if( cLeftTop.y + GetBounds().Height(  ) + 1.0f + nHeight <= cScreenRes.y )
		{
			cScrPos.y = cLeftTop.y + GetBounds().Height(  ) + 1.0f;
		}
		else
		{
			cScrPos.y = cLeftTop.y - nHeight;
		}
	}
	else
	{
		float nHeight = pcItem->GetSubMenu()->m_cSize.y;

		cScrPos.x = cLeftTop.x + GetBounds().Width(  ) + 1.0f - 4.0f;
		if( cLeftTop.y + cFrame.top - m_cItemBorders.top + nHeight <= cScreenRes.y )
		{
			cScrPos.y = cLeftTop.y + cFrame.top - m_cItemBorders.top;
		}
		else
		{
			cScrPos.y = cLeftTop.y + cFrame.bottom - nHeight + m_cItemBorders.top + pcItem->GetSubMenu()->m_cItemBorders.bottom;
		}
	}
	m_bHasOpenChilds = true;

	pcItem->GetSubMenu()->Open( cScrPos );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::TimerTick( int nID )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	OpenSelection();

}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::SelectItem( MenuItem * pcItem )
{
	MenuItem *pcPrev = FindMarked();

	if( pcItem == pcPrev )
	{
		return;
	}
	if( pcPrev != NULL )
	{
		pcPrev->SetHighlighted( false );
	}
	if( pcItem != NULL )
	{
		pcItem->SetHighlighted( true );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::SelectPrev()
{
	MenuItem *pcTmp;
	MenuItem *pcPrev = NULL;

	for( pcTmp = m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->GetNext() )
	{
		if( pcTmp->IsHighlighted() )
		{
			if( pcPrev != NULL )
			{
				SelectItem( pcPrev );
				Flush();
			}
			return;
		}
		pcPrev = pcTmp;
	}
	if( m_eLayout == ITEMS_IN_COLUMN )
	{
		Menu *pcSuper = GetSuperMenu();

		if( pcSuper != NULL && pcSuper->m_eLayout == ITEMS_IN_ROW )
		{
			Close( false, false );
		}
	}
	else
	{
		SelectItem( m_pcFirstItem );
		Flush();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::SelectNext()
{
	MenuItem *pcTmp;

	for( pcTmp = m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->GetNext() )
	{
		if( pcTmp->IsHighlighted() || pcTmp->GetNext(  ) == NULL )
		{
			if( pcTmp->GetNext() != NULL )
			{
				SelectItem( pcTmp->GetNext() );
			}
			else
			{
				SelectItem( pcTmp );
			}
			Flush();
			return;
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::Open( Point cScrPos )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	if( m_pcWindow != NULL )
	{

		return;
	}
	Point cSize = GetPreferredSize( false );
	Rect cBounds( 0, 0, cSize.x - 1, cSize.y - 1 );

	Desktop cDesktop;
	IPoint cRes = cDesktop.GetResolution();

	if( cBounds.bottom + cScrPos.y > cRes.y )
		cScrPos.y = cRes.y - cSize.y;
	if( cBounds.top + cScrPos.y < 0 )
		cScrPos.y = 0;
	if( cBounds.right + cScrPos.x > cRes.x )
		cScrPos.x = cRes.x - cSize.x;
	if( cBounds.left + cScrPos.x < 0 )
		cScrPos.x = 0;

	m_pcWindow = new MenuWindow( cBounds + cScrPos, this );

	m_pcRoot->m->m_cMutex.Lock();
	m_pcWindow->SetMutex( &m_pcRoot->m->m_cMutex );
	SetFrame( cBounds );

	m_bIsTracking = true;
	MakeFocus( true );

	SelectItem( m_pcFirstItem );
	m_pcWindow->Show();

	Flush();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::Close( bool bCloseChilds, bool bCloseParent )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
	MenuItem *pcTmp;

	for( pcTmp = m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->GetNext() )
	{
		if( pcTmp->IsHighlighted() )
		{
			pcTmp->SetHighlighted( false );
		}
	}

	if( bCloseChilds )
	{
		for( pcTmp = m_pcFirstItem; NULL != pcTmp; pcTmp = pcTmp->GetNext() )
		{
			if( pcTmp->GetSubMenu() != NULL )
			{
				pcTmp->GetSubMenu()->Close( bCloseChilds, bCloseParent );
			}
		}
	}

	if( m_pcWindow != NULL )
	{
		MenuWindow *pcWnd = m_pcWindow;

		m_pcWindow = NULL;	// Must clear this before we shutdown.
		pcWnd->ShutDown();

		Menu *pcParent = GetSuperMenu();

		if( pcParent != NULL )
		{
			// NOTE: We activate the parents window even if was not
			//       opened by the menu-system (GetWindow() vs m_pcWindow)
			Window *pcWnd = pcParent->GetWindow();

			if( pcWnd != NULL )
			{
				pcParent->MakeFocus( true );
			}
			pcParent->m_bHasOpenChilds = false;
			pcParent->m_bIsTracking = true;
		}
	}

	if( m->m_cCloseMsgTarget.IsValid() )
	{
		printf( "%d\n", m->m_cCloseMsg.GetCode() );
		m->m_cCloseMsgTarget.SendMessage( &m->m_cCloseMsg );
	}

	if( bCloseParent )
	{
		Menu *pcParent = GetSuperMenu();

		if( pcParent != NULL )
		{
			pcParent->Close( false, true );
		}
	}
	m_bIsTracking = false;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::AttachedToWindow()
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	InvalidateLayout();

}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::DetachedFromWindow()
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	Close( true, false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::WindowActivated( bool bIsActive )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	if( false == bIsActive && m_bHasOpenChilds == false )
	{
		Close( false, true );

		if( m_hTrackPort != -1 )
		{
			MenuItem *pcItem = NULL;

			if( send_msg( m_hTrackPort, 1, &pcItem, sizeof( pcItem ) ) < 0 )
			{
				dbprintf( "Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n" );
			}
		}
	}
//    s_cMutex.Unlock();
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::MouseDown( const Point & cPosition, uint32 nButtons )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	if( GetBounds().DoIntersect( cPosition ) )
	{
		MenuItem *pcItem = GetItemAt( cPosition );

		SetCloseOnMouseUp( false );

		if( pcItem != NULL )
		{
			MakeFocus( true );
			m_bIsTracking = true;
			SelectItem( pcItem );

			if( m_eLayout == ITEMS_IN_ROW )
			{
				OpenSelection();
			}
			else
			{
				StartOpenTimer( 200000 );
			}
			Flush();
		}
		OpenSelection();
	}

	else
	{
		if( m_bHasOpenChilds == false )
		{
			Close( false, true );
			if( m_hTrackPort != -1 )
			{
				MenuItem *pcItem = NULL;

				if( send_msg( m_hTrackPort, 1, &pcItem, sizeof( pcItem ) ) < 0 )
				{
					dbprintf( "Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n" );
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

void Menu::EndSession( MenuItem * pcSelItem )
{
	Menu *pcSuper = GetSuperMenu();

	if( NULL != pcSuper )
	{
		Window *pcWindow = pcSuper->GetWindow();

		if( NULL != pcWindow )
		{
			Message cMsg( M_MENU_EVENT );

			cMsg.AddPointer( "_item", pcSelItem /* GetSuperItem() */  );

			pcWindow->PostMessage( &cMsg, pcSuper );
		}
	}
	if( m_hTrackPort != -1 )
	{
		if( send_msg( m_hTrackPort, 1, &pcSelItem, sizeof( &pcSelItem ) ) < 0 )
		{
			dbprintf( "Error: Menu::EndSession() failed to send message to m_hTrackPort\n" );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	if( GetBounds().DoIntersect( cPosition ) )
	{
		MenuItem *pcItem = FindMarked();

		if( NULL != pcItem && pcItem == GetItemAt( cPosition ) && NULL == pcItem->GetSubMenu() )
		{
			if( pcItem->IsEnabled() )	//if enabled it will invoke the message and then close
			{	//else it will not do anything
				pcItem->Invoke();
				Close( false, true );
			}
		}

	}

	else
	{
		if( m_bCloseOnMouseUp )
		{
			Close( false, true );
			if( m_hTrackPort != -1 )
			{
				MenuItem *pcItem = NULL;

				if( send_msg( m_hTrackPort, 1, &pcItem, sizeof( pcItem ) ) < 0 )
				{
					dbprintf( "Error: Menu::WindowActivated() failed to send message to m_hTrackPort\n" );
				}
			}
		}
	}
	SetCloseOnMouseUp( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::MouseMove( const Point & cPosition, int nCode, uint32 nButtons, Message * pcData )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	if( nButtons & 0x01 )
	{
		SetCloseOnMouseUp( true );
	}
	if( GetBounds().DoIntersect( cPosition ) == false )
	{
		Menu *pcParent = GetSuperMenu();

		if( pcParent != NULL )
		{
			pcParent->MouseMove( pcParent->ConvertFromScreen( ConvertToScreen( cPosition ) ), nCode, nButtons, pcData );
		}
	}
	else
	{
		if( m_bIsTracking )
		{
			MenuItem *pcItem = GetItemAt( cPosition );

			if( pcItem != NULL )
			{
				SelectItem( pcItem );
				if( m_eLayout == ITEMS_IN_ROW )
				{
					OpenSelection();
				}
				else
				{
					StartOpenTimer( 200000 );
				}
				Flush();
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

void Menu::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );

	switch ( pzString[0] )
	{
	case VK_UP_ARROW:
		if( m_eLayout == ITEMS_IN_ROW )
		{
		}
		else
		{
			SelectPrev();
			StartOpenTimer( 1000000 );
		}
		break;
	case VK_DOWN_ARROW:
		if( m_eLayout == ITEMS_IN_ROW )
		{
			OpenSelection();
		}
		else
		{
			SelectNext();
			StartOpenTimer( 1000000 );
		}
		break;
	case VK_LEFT_ARROW:
		if( m_eLayout == ITEMS_IN_ROW )
		{
			SelectPrev();
			OpenSelection();
		}
		else
		{
			Menu *pcSuper = GetSuperMenu();

			if( pcSuper != NULL )
			{
				if( pcSuper->m_eLayout == ITEMS_IN_COLUMN )
				{
					Close( false, false );
				}
				else
				{
					pcSuper->SelectPrev();
					pcSuper->OpenSelection();
				}
			}
		}
		break;
	case VK_RIGHT_ARROW:
		if( m_eLayout == ITEMS_IN_ROW )
		{
			SelectNext();
			OpenSelection();
		}
		else
		{
			Menu *pcSuper = GetSuperMenu();

			if( pcSuper != NULL )
			{
				MenuItem *pcItem = FindMarked();

				if( pcItem != NULL && pcItem->GetSubMenu() != NULL )
				{
					OpenSelection();
				}
				else if( pcSuper->m_eLayout == ITEMS_IN_ROW )
				{
					pcSuper->SelectNext();
					pcSuper->OpenSelection();
				}
			}
		}
		break;
	case VK_ENTER:
		{
			MenuItem *pcItem = FindMarked();

			if( pcItem != NULL )
			{
				if( pcItem->GetSubMenu() != NULL )
				{
					OpenSelection();
				}
				else
				{
					pcItem->Invoke();
					Close( false, true );
				}
			}
			break;
		}
	case 27:		// ESC
		Close( false, false );
		Flush();
		break;
	default:
		View::KeyDown( pzString, pzRawString, nQualifiers );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::Paint( const Rect & cUpdateRect )
{
	AutoLocker __lock__( &m_pcRoot->m->m_cMutex );
	MenuItem *pcItem;

	SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
	FillRect( GetBounds() );


	for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
		pcItem->Draw();

	DrawFrame( GetBounds(), FRAME_RAISED | FRAME_THIN | FRAME_TRANSPARENT );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( const char *pzLabel, Message * pcMessage )
{
	return ( AddItem( new MenuItem( pzLabel, pcMessage ) ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( MenuItem * pcItem )
{
	return ( AddItem( pcItem, m_nItemCount ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( MenuItem * pcItem, int nIndex )
{
	bool bResult = false;

	pcItem->SetSuperMenu( this );

	if( nIndex == 0 )
	{
		pcItem->SetNext( m_pcFirstItem );
		m_pcFirstItem = pcItem;
		bResult = true;
	}
	else
	{
		MenuItem *pcPrev;
		int i;

		for( i = 1, pcPrev = m_pcFirstItem; i < nIndex && NULL != pcPrev; ++i )
		{
			pcPrev = pcPrev->GetNext();
		}
		if( pcPrev )
		{
			pcItem->SetNext( pcPrev->GetNext() );
			pcPrev->SetNext( pcItem );
			bResult = true;
		}
	}

	if( bResult )
	{
		if( pcItem->GetSubMenu() != NULL )
		{
			pcItem->GetSubMenu()->SetRoot( m_pcRoot );
		}
		m_nItemCount++;
		InvalidateLayout();
	}

	return ( bResult );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( Menu * pcMenu )
{
	pcMenu->SetRoot( m_pcRoot );
	return ( AddItem( pcMenu, m_nItemCount ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::AddItem( Menu * pcMenu, int nIndex )
{
	pcMenu->SetRoot( m_pcRoot );
	return ( AddItem( new MenuItem( pcMenu, NULL ), nIndex ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem *Menu::RemoveItem( int nIndex )
{
	MenuItem *pcTmp;
	MenuItem *pcPrev = NULL;
	MenuItem *pcItem = NULL;

	for( pcTmp = m_pcFirstItem; pcTmp != NULL; pcPrev = pcTmp, pcTmp = pcTmp->GetNext() )
	{
		if( nIndex == 0 )
		{
			pcItem = pcTmp;
			pcTmp = pcItem->GetNext();

			if( pcPrev )
				pcPrev->SetNext( pcTmp );
			else
				m_pcFirstItem = pcTmp;

			if( pcItem->GetSubMenu() != NULL )
			{
				pcItem->GetSubMenu()->SetRoot( pcItem->GetSubMenu(  ) );
			}

			m_nItemCount--;
			InvalidateLayout();
			break;
		}
		nIndex--;
	}

	return ( pcItem );

/* What was the point with this code ??? */

/*
    MenuItem** ppcTmp;
    MenuItem*  pcItem = NULL;
  
    for ( ppcTmp = &m_pcFirstItem ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->GetNext() ) {
	if ( nIndex == 0 ) {
	    pcItem = *ppcTmp;
	    *ppcTmp = pcItem->GetNext();
	    if ( pcItem->GetSubMenu() != NULL ) {
		pcItem->GetSubMenu()->SetRoot( pcItem->GetSubMenu() );
	    }
	    m_nItemCount--;
	    InvalidateLayout();
	    break;
	}
	nIndex--;
    }
    return( pcItem );*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::RemoveItem( MenuItem * pcItem )
{
	MenuItem *pcTmp;
	MenuItem *pcPrev = NULL;

	for( pcTmp = m_pcFirstItem; pcTmp != NULL; pcPrev = pcTmp, pcTmp = pcTmp->GetNext() )
	{
		if( pcItem == pcTmp )
		{
			pcItem = pcTmp;
			pcTmp = pcItem->GetNext();

			if( pcPrev )
				pcPrev->SetNext( pcTmp );
			else
				m_pcFirstItem = pcTmp;

			if( pcItem->GetSubMenu() != NULL )
			{
				pcItem->GetSubMenu()->SetRoot( pcItem->GetSubMenu(  ) );
			}

			delete pcItem;

			m_nItemCount--;
			InvalidateLayout();
			return true;
		}
	}
	return false;

/*
    MenuItem** ppcTmp;
  
    for ( ppcTmp = &m_pcFirstItem ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->GetNext() ) {
	if ( *ppcTmp == pcItem ) {
	    *ppcTmp = pcItem->GetNext();
	    m_nItemCount--;
	    InvalidateLayout();
	    if ( pcItem->GetSubMenu() != NULL ) {
		pcItem->GetSubMenu()->SetRoot( pcItem->GetSubMenu() );
	    }
	    return( true );
	}
    }
    return( false );*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Menu::RemoveItem( Menu * pcMenu )
{
	MenuItem *pcTmp;
	MenuItem *pcPrev = NULL;
	MenuItem *pcItem = NULL;

	for( pcTmp = m_pcFirstItem; pcTmp != NULL; pcPrev = pcTmp, pcTmp = pcTmp->GetNext() )
	{
		if( pcTmp->GetSubMenu() == pcMenu )
		{
			pcItem = pcTmp;
			pcTmp = pcItem->GetNext();

			if( pcPrev )
				pcPrev->SetNext( pcTmp );
			else
				m_pcFirstItem = pcTmp;

			pcMenu->SetRoot( pcMenu );

			delete pcItem;

			m_nItemCount--;
			InvalidateLayout();
			return true;
		}
	}
	return false;

/*
    MenuItem** ppcTmp;
    MenuItem*  pcItem = NULL;
  
    for ( ppcTmp = &m_pcFirstItem ; *ppcTmp != NULL ; ppcTmp = &(*ppcTmp)->GetNext() ) {
	pcItem = *ppcTmp;
	if ( pcItem->GetSubMenu() == pcMenu ) {
	    *ppcTmp = pcItem->GetNext();
	    pcMenu->SetRoot( pcMenu );
	    m_nItemCount--;
	    InvalidateLayout();
	    return( true );
	}
    }
    return( false );*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem *Menu::GetItemAt( Point cPosition ) const
{
	MenuItem *pcItem;

	for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
	{
		if( pcItem->GetFrame().DoIntersect( cPosition ) )
		{
			break;
		}
	}
	return ( pcItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem *Menu::GetItemAt( int nIndex ) const
{
	MenuItem *pcItem = NULL;

	if( nIndex >= 0 && nIndex < m_nItemCount )
	{
		for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
		{
			if( 0 == nIndex )
			{
				return ( pcItem );
			}
			nIndex--;
		}
	}
	return ( NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu *Menu::GetSubMenuAt( int nIndex ) const
{
	MenuItem *pcItem = GetItemAt( nIndex );

	if( NULL != pcItem )
	{
		return ( pcItem->GetSubMenu() );
	}
	else
	{
		return ( NULL );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Menu::GetItemCount() const
{
	return ( m_nItemCount );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Menu::GetIndexOf( MenuItem * pcItem ) const
{
	MenuItem *pcPtr;
	int nIndex = 0;

	for( pcPtr = m_pcFirstItem; NULL != pcPtr; pcPtr = pcPtr->GetNext() )
	{
		if( pcPtr == pcItem )
		{
			return ( nIndex );
		}
		nIndex++;
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Menu::GetIndexOf( Menu * pcMenu ) const
{
	MenuItem *pcPtr;
	int nIndex = 0;

	for( pcPtr = m_pcFirstItem; NULL != pcPtr; pcPtr = pcPtr->GetNext() )
	{
		if( pcPtr->GetSubMenu() == pcMenu )
		{
			return ( nIndex );
		}
		nIndex++;
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem *Menu::FindItem( int nCode ) const
{
	MenuItem *pcItem = NULL;

	for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
	{
		if( pcItem->GetSubMenu() != NULL )
		{
			MenuItem *pcTmp = pcItem->GetSubMenu()->FindItem( nCode );

			if( pcTmp != NULL )
			{
				return ( pcTmp );
			}
		}
		else
		{
			Message *pcMsg = pcItem->GetMessage();

			if( pcMsg != NULL && pcMsg->GetCode() == nCode )
			{
				return ( pcItem );
			}
		}
	}
	return ( NULL );
}

void Menu::SetCloseOnMouseUp( bool bFlag )
{
	MenuItem *pcItem;

	m_bCloseOnMouseUp = bFlag;

	for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
	{
		if( pcItem->GetSubMenu() != NULL )
		{
			pcItem->GetSubMenu()->SetCloseOnMouseUp( bFlag );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem *Menu::FindItem( const char *pzName ) const
{
	MenuItem *pcItem;

	for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
	{
		if( pcItem->GetSubMenu() != NULL )
		{
			MenuItem *pcTmp = pcItem->GetSubMenu()->FindItem( pzName );

			if( pcTmp != NULL )
			{
				return ( pcTmp );
			}
		}
		else
		{
			if( NULL != pcItem->GetLabel() && strcmp( pcItem->GetLabel(  ), pzName ) == 0 )
			{
				break;
			}
		}
	}
	return ( pcItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Menu::SetTargetForItems( Handler * pcTarget )
{
	MenuItem *pcItem;

	for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
	{
		if( NULL == pcItem->GetSubMenu() )
		{
			pcItem->SetTarget( pcTarget );
		}
		else
		{
			pcItem->GetSubMenu()->SetTargetForItems( pcTarget );
		}
	}
	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Menu::SetTargetForItems( Messenger cMessenger )
{
	MenuItem *pcItem;

	for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
	{
		if( NULL == pcItem->GetSubMenu() )
		{
			pcItem->SetTarget( cMessenger );
		}
		else
		{
			pcItem->GetSubMenu()->SetTargetForItems( cMessenger );
		}
	}
	return ( 0 );
}

void Menu::SetCloseMessage( const Message & cMsg )
{
	m->m_cCloseMsg = cMsg;
}

void Menu::SetCloseMsgTarget( const Messenger & cTarget )
{
	m->m_cCloseMsgTarget = cTarget;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem *Menu::FindMarked() const
{
	MenuItem *pcItem;

	for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
	{
		if( pcItem->IsHighlighted() )
		{
			break;
		}
	}
	return ( pcItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Menu *Menu::GetSuperMenu() const
{
	if( NULL != m_pcSuperItem )
	{
		return ( m_pcSuperItem->GetSuperMenu() );
	}
	else
	{
		return ( NULL );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem *Menu::GetSuperItem()
{
	return ( m_pcSuperItem );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point Menu::GetPreferredSize( bool bLargest ) const
{
	return ( m_cSize + Point( 2.0f, 2.0f ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Menu::InvalidateLayout()
{
	MenuItem *pcItem;

	m_cSize = Point( 0.0f, 0.0f );

	switch ( m_eLayout )
	{
	case ITEMS_IN_ROW:
		for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
		{
			Point cSize = pcItem->GetContentSize();

			pcItem->SetFrame( Rect( m_cSize.x + m_cItemBorders.left + 1, m_cItemBorders.top + 1, m_cSize.x + cSize.x + m_cItemBorders.left - 1, cSize.y + m_cItemBorders.bottom - 1 ) );

			m_cSize.x += cSize.x + m_cItemBorders.left + m_cItemBorders.right;

			if( cSize.y > m_cSize.y )
				m_cSize.y = cSize.y;
		}
		m_cSize.y += m_cItemBorders.top + m_cItemBorders.bottom /*+ 2 */ ;

		break;

	case ITEMS_IN_COLUMN:
		for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
		{
			Point cSize = pcItem->GetContentSize();

			pcItem->SetFrame( Rect( m_cItemBorders.left, m_cSize.y + m_cItemBorders.top, cSize.x + m_cItemBorders.right - 1, m_cSize.y + cSize.y + m_cItemBorders.top - 1 ) );

			m_cSize.y += cSize.y + m_cItemBorders.top + m_cItemBorders.bottom;

			if( cSize.x + m_cItemBorders.left + m_cItemBorders.right > m_cSize.x )
				m_cSize.x = cSize.x + m_cItemBorders.left + m_cItemBorders.right;
		}

		for( pcItem = m_pcFirstItem; NULL != pcItem; pcItem = pcItem->GetNext() )
		{
			Rect frame = pcItem->GetFrame();

			frame.right = m_cSize.x - 2;
			pcItem->SetFrame( frame );
		}

		break;

	case ITEMS_CUSTOM_LAYOUT:
		dbprintf( "Error: ITEMS_CUSTOM_LAYOUT not supported\n" );
		break;
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

MenuItem *Menu::Track( const Point & cScreenPos )
{
	m_hTrackPort = create_port( "menu_track_port", 1 );
	if( m_hTrackPort < 0 )
	{
		return ( NULL );
	}
	Point cSize = GetPreferredSize( false );
	Rect cBounds( 0, 0, cSize.x - 1, cSize.y - 1 );

	m_pcWindow = new MenuWindow( cBounds + cScreenPos, this );
	m_pcRoot->m->m_cMutex.Lock();
	m_pcWindow->SetMutex( &m_pcRoot->m->m_cMutex );

	SetFrame( cBounds );

	m_pcWindow->Show();

//    if ( m_pcWindow->Lock() == 0 )
//    {
	MakeFocus( true );
//      m_pcWindow->Unlock();
//    }
	m_bIsTracking = true;

	uint32 nCode;
	MenuItem *pcSelItem;

	get_msg( m_hTrackPort, &nCode, &pcSelItem, sizeof( &pcSelItem ) );

	assert( m_pcWindow == NULL );
	delete_port( m_hTrackPort );
	m_hTrackPort = -1;
	return ( pcSelItem );
}

void Menu::SetEnable( bool bEnabled )
{
	m_bEnabled = bEnabled;

	for( int i = 0; i < GetItemCount(); i++ )
		GetItemAt( i )->SetEnable( bEnabled );
}

bool Menu::IsEnabled()
{
	return m_bEnabled;
}
