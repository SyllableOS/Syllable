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

#include <gui/checkmenu.h>
#include <util/message.h>

using namespace os;

Bitmap *CheckMenu::s_pcCheckBitmap = NULL;

#define CHECK_W 10
#define CHECK_H 10

static uint8 nCheckData[] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
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
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
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
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CheckMenu::CheckMenu( const String& cLabel, Message * pcMsg, bool bChecked )
:MenuItem( cLabel, pcMsg )
{
	m_Highlighted = false;
	m_Enabled = true;
	m_IsChecked = bChecked;
	m_bIsSelectable = true;

	if( s_pcCheckBitmap == NULL )
	{
		Rect cCheckBitmapRect;

		cCheckBitmapRect.left = 0;
		cCheckBitmapRect.top = 0;
		cCheckBitmapRect.right = CHECK_W;
		cCheckBitmapRect.bottom = CHECK_H;

		s_pcCheckBitmap = new Bitmap( (int)cCheckBitmapRect.Width(), (int)cCheckBitmapRect.Height(  ), CS_RGBA32, Bitmap::SHARE_FRAMEBUFFER );
		memcpy( s_pcCheckBitmap->LockRaster(), nCheckData, ( unsigned int )( cCheckBitmapRect.Width(  ) * cCheckBitmapRect.Height(  ) * 4 ) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CheckMenu::CheckMenu( Menu * pcMenu, Message * pcMsg, bool bChecked )
:MenuItem( pcMenu, pcMsg )
{
	m_Highlighted = false;
	m_Enabled = true;
	m_IsChecked = bChecked;
	m_bIsSelectable = true;

	if( s_pcCheckBitmap == NULL )
	{
		Rect cCheckBitmapRect;

		cCheckBitmapRect.left = 0;
		cCheckBitmapRect.top = 0;
		cCheckBitmapRect.right = CHECK_W;
		cCheckBitmapRect.bottom = CHECK_H;

		s_pcCheckBitmap = new Bitmap( (int)cCheckBitmapRect.Width(), (int)cCheckBitmapRect.Height(  ), CS_RGBA32, Bitmap::SHARE_FRAMEBUFFER );
		memcpy( s_pcCheckBitmap->LockRaster(), nCheckData, ( unsigned int )( cCheckBitmapRect.Width(  ) * cCheckBitmapRect.Height(  ) * 4 ) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

CheckMenu::~CheckMenu()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void CheckMenu::Draw()
{
	Menu *pcMenu = GetSuperMenu();

	if( pcMenu == NULL )
	{
		return;
	}

	const String& cLabel = GetLabel();

	Rect cFrame = GetFrame();

	font_height sHeight;

	pcMenu->GetFontHeight( &sHeight );

	if( m_Highlighted )
	{
		pcMenu->SetFgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
	}
	else
	{
		pcMenu->SetFgColor( get_default_color( COL_MENU_BACKGROUND ) );
	}

	pcMenu->FillRect( GetFrame() );

	if( m_Highlighted )
	{
		pcMenu->SetFgColor( get_default_color( COL_SEL_MENU_TEXT ) );
		pcMenu->SetBgColor( get_default_color( COL_SEL_MENU_BACKGROUND ) );
	}
	else
	{
		pcMenu->SetFgColor( get_default_color( COL_MENU_TEXT ) );
		pcMenu->SetBgColor( get_default_color( COL_MENU_BACKGROUND ) );
	}

	float vCharHeight = sHeight.ascender + sHeight.descender + sHeight.line_gap;
	float y = cFrame.top + ( cFrame.Height() + 1.0f ) / 2 - vCharHeight / 2 + sHeight.ascender + sHeight.line_gap * 0.5f;

	pcMenu->DrawString( Point( cFrame.left + 16, y ), cLabel );

	// If the menu is checked, draw the checkmark glyph on the right of the label
	if( m_IsChecked )
	{
		Rect cSourceRect = s_pcCheckBitmap->GetBounds();
		Rect cTargetRect;
		Rect cFrame = GetFrame();

		cTargetRect = cSourceRect.Bounds() + Point( cFrame.left + 2.0f, cFrame.top + ( cFrame.Height(  ) / 2 ) - 5.0f );

		pcMenu->SetDrawingMode( DM_OVER );
		pcMenu->DrawBitmap( s_pcCheckBitmap, cSourceRect, cTargetRect );
		pcMenu->SetDrawingMode( DM_COPY );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void CheckMenu::SetHighlighted( bool bHighlighted )
{
	m_Highlighted = bHighlighted;
	MenuItem::SetHighlighted( bHighlighted );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point CheckMenu::GetContentSize() const
{
	Menu *pcMenu = GetSuperMenu();

	if( pcMenu != NULL )
	{
		font_height sHeight;

		pcMenu->GetFontHeight( &sHeight );

		return ( Point( pcMenu->GetStringWidth( GetLabel() ) + 16, sHeight.ascender + sHeight.descender + sHeight.line_gap ) );
	}

	return ( Point( 0.0f, 0.0f ) );
}

/** Find out if the menu is currently selected
 * \par Description:
 *	Find out if a menu is checked
 * \return
 * true if the menu is checked, false if the menu is unchecked
 * \sa void SetChecked(bool bChecked)
 * \author Kristian Van Der Vliet (vanders@blueyonder.co.uk)
 *****************************************************************************/

bool CheckMenu::IsChecked() const
{
	return ( m_IsChecked );
}

/** Check or uncheck a menu
 * \par Description:
 *	Set a menu as checked, or uncheck a checked menu
 * \param bChecked - True to check the menu, or false to uncheck
 * \sa bool IsChecked( )
 * \author Kristian Van Der Vliet (vanders@blueyonder.co.uk)
 *****************************************************************************/

void CheckMenu::SetChecked( bool bChecked )
{
	m_IsChecked = bChecked;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool CheckMenu::Invoked( Message * pcMessage )
{
	if( m_IsChecked )
		m_IsChecked = false;
	else
		m_IsChecked = true;

	pcMessage->AddBool( "status", m_IsChecked );
	return true;
}


bool CheckMenu::IsSelectable() const
{
	return m_bIsSelectable;
}


