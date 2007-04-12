/*
 *  aterm - x-term like terminal emulator for AtheOS
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <termios.h>

#include <util/application.h>
#include <gui/window.h>
#include <gui/scrollbar.h>
#include <util/message.h>
#include <util/clipboard.h>
#include <atheos/time.h>

#include <gui/requesters.h>

#include "tview.h"
#include "swindow.h"
#include "messages.h"

#include <macros.h>

#include <string>

#define POINTER1_WIDTH  7
#define POINTER1_HEIGHT 14
#define POINTER2_WIDTH  9
#define POINTER2_HEIGHT 16

static uint8 g_anMouseImg1[] = {
	0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02
};

static uint8 g_anMouseImg2[] = {
	0x03, 0x03, 0x03, 0x03, 0x00, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x03, 0x03, 0x03,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x00, 0x00, 0x00,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x02, 0x02, 0x02, 0x03, 0x02, 0x02, 0x02, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x00, 0x03, 0x03, 0x03, 0x03,
};


volatile bool g_bDontRender = false;


int g_nDefaultAttribs = 0x07;	/* These attributes will be replaced by custom colors. */

#define DEBUG( level, fmt, args... ) do { if ( level < g_nDebugLevel ) printf( fmt, ## args ); } while(0)

const char *g_pzAboutText = "ATerm is a virtual terminal for Syllable.\n" "\n" "Version " VERSION "\n\n" "Copyright (C) 2004 Damien Daneels, Rick Caudill and Henrik Isaksson.\n" "Copyright (C) 1999 - 2001 Kurt Skauen\n\n" "This program is free software; you can redistribute it and/or modify\n" "it under the terms of the GNU General Public License as published by\n" "the Free Software Foundation; either version 2 of the License, or\n" "(at your option) any later version.\n" "\n" "This program is distributed in the hope that it will be useful,\n" "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" "GNU General Public License for more details.\n" "\n" "You should have received a copy of the GNU General Public License\n" "along with this program; if not, write to the Free Software\n" "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA";


/**
 * Get text color
 *
 * Get text color for given attributes. If attribute is equal to attribute mask,
 * this method will returns the user normal text color.
 */
Color32_s TermView::GetAttrFgColor( int nAttrib )
{
	if( ( nAttrib & 0x0f ) == ( g_nDefaultAttribs & 0x0f ) )
	{
		return m_pcSettings->GetNormalTextColor();
	}
	else
	{
		return TermSettings::GetPaletteColor( nAttrib & 0x0f );
	}
}

/**
 * Get background color
 *
 * Get background color for given attributes. If attribute is equal to attribute mask,
 * this method will returns the user normal background color.
 */
Color32_s TermView::GetAttrBgColor( int nAttrib )
{
	if( ( nAttrib & 0xf0 ) == ( g_nDefaultAttribs & 0xf0 ) )
	{
		return m_pcSettings->GetNormalBackgroundColor();
	}
	else
	{
		return TermSettings::GetPaletteColor( nAttrib >> 4 );
	}
}

/**
 *
 */
TextLine::TextLine( int32 nChar, int nAttrib )
{
	m_nMaxLength = 200;
	m_nCurLength = 0;

	m_pCharMap = new int32[m_nMaxLength];
	m_pAttribMap = new int16[m_nMaxLength];

	Clear( 0, MAX, nChar, nAttrib );
}

/**
 *
 */
TextLine::~TextLine()
{
	delete[]m_pCharMap;
	delete[]m_pAttribMap;
}

/**
 *
 */
void TextLine::Clear( int nStart, int nEnd, int32 nChar, int nAttr )
{
	int i;

	if( nStart < 0 )
	{
		nStart = 0;
	}
	if( nStart >= m_nMaxLength )
	{
		nStart = m_nMaxLength - 1;
	}

	if( nEnd >= m_nMaxLength )
	{
		nEnd = m_nMaxLength - 1;
	}

	if( nEnd < 0 )
	{
		nEnd = 0;
	}

	for( i = nStart; i <= nEnd; ++i )
	{
		m_pCharMap[i] = nChar;
		m_pAttribMap[i] = nAttr;
	}
}

/**
 *
 */
TermView::TermView( const Rect & cFrame, TermSettings * pcSettings, const char *pzTitle, uint32 nResizeMask, uint32 nFlags ):View( cFrame, pzTitle, nResizeMask, nFlags )
{
	m_pcSettings = pcSettings;

	m_cMaxCharMapSize = IPoint( 0, 0 );
	m_cCurCharMapSize = IPoint( 0, 0 );

	m_nScrollTop = 0;
	m_nScrollBottom = -1;	// Force initializing in FrameSized()

	m_nCurrentBuffer = 0;
	m_nKeyPadMode = 0;
	m_nEscStringSize = 0;

	m_pBuffer = NULL;
	m_pPrevBuffer = NULL;
	m_pabModifiedLines = NULL;

	m_nCurAttrib = g_nDefaultAttribs;

	m_nMaxLineCount = 0;
	m_nTotLineCnt = 0;

	m_nScrollPos = 0;
	m_bModified = true;
	m_nCharSet = 0;
	m_bInsertMode = false;
	m_bIsSelecting = false;
	m_bIsSelectingWords = false;
	m_bIsSelRangeValid = false;
	m_nLastClickTime = 0;
	m_bAlternateCursorKeys = false;

	m_pcMenu = new Menu( Rect( 0, 0, 1, 1 ), "unnamed popup menu", ITEMS_IN_COLUMN );
	m_pcMenu->AddItem( "About...", new Message( M_MENU_ABOUT ) );
	m_pcMenu->AddItem( "Settings...", new Message( M_MENU_SETTINGS ) );
	m_pcMenu->AddItem( new MenuSeparator() );
	m_pcMenu->AddItem( "Copy", new Message( M_MENU_COPY ), "CTRL+C" );
	m_pcMenu->AddItem( "Paste", new Message( M_MENU_PASTE ), "CTRL+V" );
}


/**
 *
 */
TermView::~TermView()
{
}

/**
 *
 */
void TermView::SetScrollWindow( int y1, int y2 )
{
	if( y1 < 0 )
		y1 = 0;
	if( y2 >= m_cCurCharMapSize.y )
		y2 = m_cCurCharMapSize.y - 1;


	m_nScrollTop = y1;
	m_nScrollBottom = y2;
}

/**
 *
 */
TextLine *TermView::GetLineBuf( int y, bool bWidthScroll ) const
{
	int nOffset = ( bWidthScroll ) ? m_nScrollPos : 0;

	if( ( m_cCurCharMapSize.y + nOffset - y - 1 ) < 0 || ( m_cCurCharMapSize.y + nOffset - y - 1 ) >= m_nTotLineCnt )
	{
		return ( NULL );
	}
	return ( m_pBuffer[m_cCurCharMapSize.y + nOffset - y - 1] );
}

/**
 *
 */
TextLine *TermView::GetPrevLineBuf( int y ) const
{
	if( y < 0 || y >= m_cCurCharMapSize.y )
	{
		dbprintf( "ERROR : GetPrevLineBuf() - Attempt to get line %d. Height = %d\n", y, m_cCurCharMapSize.y );
		return ( NULL );
		y = 0;
	}
	return ( m_pPrevBuffer[m_cCurCharMapSize.y - y - 1] );
}

/**
 *
 */
void TermView::InvalidateLine( int y )
{
	if( y >= 0 && y < m_cCurCharMapSize.y )
	{
		m_pabModifiedLines[y] = true;
	}
}

/**
 *
 */
void TermView::AttachedToWindow( void )
{
	Font *pcFont = GetFont();

	if( NULL != pcFont )
	{
		pcFont->SetProperties( DEFAULT_FONT_FIXED );
		Flush();
	}

	GetFontHeight( &m_sFontHeight );

	m_cCharSize.x = GetStringWidth( "M" );
	m_cCharSize.y = m_sFontHeight.ascender + m_sFontHeight.descender + m_sFontHeight.line_gap + 1;

	g_pcScrollBar->SetSteps( 2, GetBounds().Height(  ) / m_cCharSize.y - 1.0f );

	MakeFocus( true );
}

/**
 *
 */
void TermView::ExpandCharMap( IPoint cNewSize )
{
	int i;

	if( 0 != m_nMaxLineCount )
	{
		m_cCsrPos.y += cNewSize.y - m_cCurCharMapSize.y;
	}

	if( cNewSize.y > 0 )
	{
		if( cNewSize.y > m_nTotLineCnt )
		{
			int nNewMaxHeight = cNewSize.y + 100;
			TextLine **pNewBuf = new TextLine *[nNewMaxHeight];

			memcpy( pNewBuf, m_pBuffer, sizeof( m_pBuffer[0] ) * m_nTotLineCnt );

			delete[]m_pBuffer;

			m_pBuffer = pNewBuf;

			if( cNewSize.y > m_nTotLineCnt )
			{
				for( i = m_nTotLineCnt; i < cNewSize.y; ++i )
				{
					m_pBuffer[i] = new TextLine( ' ', m_nCurAttrib );
				}
				m_nTotLineCnt = cNewSize.y;

				if( NULL != g_pcScrollBar )
				{
					g_pcScrollBar->SetProportion( float ( cNewSize.y ) / float ( m_nTotLineCnt ) );

					g_pcScrollBar->SetMinMax( 0, 0 );
					g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - cNewSize.y );
				}
			}

			m_nMaxLineCount = nNewMaxHeight;
		}

		for( i = 0; i < m_cCurCharMapSize.y; ++i )
		{
			delete m_pPrevBuffer[i];
		}
		delete[]m_pPrevBuffer;

		m_pPrevBuffer = new TextLine *[cNewSize.y];

		for( i = 0; i < cNewSize.y; ++i )
		{
			m_pPrevBuffer[i] = new TextLine( -1, 0 );
		}

		delete[]m_pabModifiedLines;
		m_pabModifiedLines = new bool[cNewSize.y];
		memset( m_pabModifiedLines, true, sizeof( bool ) * cNewSize.y );
	}
	m_cCurCharMapSize = cNewSize;
}

/**
 * Generic terminal view resizing handler
 *
 * When terminal view is resized, changes are immediatly reflected to the settings structure.
 */
void TermView::FrameSized( const Point & cDelta )
{
	Rect cBounds = GetBounds();

	int nWidth  = std::max( 10,(int)((cBounds.Width() + 1.0f ) / m_cCharSize.x));
	int nHeight = (int)((cBounds.Height() + 1.0f ) / m_cCharSize.y);
	IPoint cNewSize(nWidth,nHeight);


	if( 0 == m_nScrollTop && m_nScrollBottom == m_cCurCharMapSize.y - 1 || -1 == m_nScrollBottom )
	{
		m_nScrollBottom = cNewSize.y - 1;
	}

	g_bDontRender = true;
	ExpandCharMap( cNewSize );
	m_nScrollPos = 0;
	g_bDontRender = false;

	g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );

	struct winsize sWinSize;

	sWinSize.ws_col = m_cCurCharMapSize.x;
	sWinSize.ws_row = m_cCurCharMapSize.y;
	sWinSize.ws_xpixel = int ( cBounds.Width() ) + 1;
	sWinSize.ws_ypixel = int ( cBounds.Height() ) + 1;

	ioctl( g_nMasterPTY, TIOCSWINSZ, &sWinSize );
	g_bDontRender = false;

	RefreshDisplay( true );

	/* Save new size to settings */
	m_pcSettings->SetSize( IPoint( m_cCurCharMapSize.x, m_cCurCharMapSize.y ) );

/*    printf("m_pcSettings "); m_pcSettings->Print();*/
}


/**
 * Copy selection to clipboard
 *
 */
void TermView::Copy()
{
	IPoint cPnt1;
	IPoint cPnt2;
	int nMinX;
	int nMaxX;

	if( m_bIsSelRangeValid == false )
	{
		return;
	}

	SortSelection( &cPnt1, &cPnt2 );

	std::string cBuffer;

	for( int y = cPnt1.y; y <= cPnt2.y; ++y )
	{
		TextLine *pcLineBuf = GetLineBuf( y );

		if( pcLineBuf == NULL )
		{
			continue;
		}
		if( m_bIsSelRangeRect )
		{
			nMinX = cPnt1.x;
			nMinX = cPnt2.x;

			if( cPnt1.x < cPnt2.x )
			{
				nMinX = cPnt1.x;
			nMaxX = cPnt2.x;
			}
			else
			{
				nMinX = cPnt2.x;
				nMaxX = cPnt1.x;
			}
		}
		else
		{
			if( y == cPnt1.y )
			{
				nMinX = cPnt1.x;
			}
			else
			{
				nMinX = 0;
			}
			if( y == cPnt2.y )
			{
				nMaxX = cPnt2.x;
			}
			else
			{
				nMaxX = pcLineBuf->m_nMaxLength - 1;
			}
		}

		if( nMinX < 0 )
		{
			nMinX = 0;
		}
		else if( nMinX >= pcLineBuf->m_nMaxLength )
		{
			nMinX = pcLineBuf->m_nMaxLength - 1;
		}
		if( nMaxX < 0 )
		{
			nMaxX = 0;
		}
		else if( nMaxX >= pcLineBuf->m_nMaxLength )
		{
			nMaxX = pcLineBuf->m_nMaxLength - 1;
		}

		for( int x = nMaxX; x >= nMinX; --x )
		{
			if( !isspace( pcLineBuf->m_pCharMap[x] ) )
			{
				nMaxX = x;
				break;
			}
		}
		for( int q = nMinX; q < nMaxX + 1; q++ )
		{
			char bfr[8];
			uint nLen = unicode_to_utf8( bfr, pcLineBuf->m_pCharMap[q] );

			cBuffer.append( bfr, nLen );
		}
		if( y != cPnt2.y )
		{
			cBuffer += '\n';
		}
	}
	Clipboard cClipboard;

	cClipboard.Lock();
	cClipboard.Clear();
	Message *pcData = cClipboard.GetData();

	pcData->AddString( "text/plain", cBuffer.c_str() );
	cClipboard.Commit();
	cClipboard.Unlock();

	RemoveSelection();
	m_bModified = true;

	m_bIsSelecting = false;
	m_bIsSelRangeValid = false;
}


/**
 * Paste clipboard
 *
 */
void TermView::Paste()
{
	const char *pzBuffer;
	int nError;
	Clipboard cClipboard;

	cClipboard.Lock();
	Message *pcData = cClipboard.GetData();

	nError = pcData->FindString( "text/plain", &pzBuffer );
	cClipboard.Unlock();

	if( nError == 0 )
	{
		write( g_nMasterPTY, pzBuffer, strlen( pzBuffer ) );
	}
}

/**
 * General message handle
 *
 */
void TermView::HandleMessage( Message * pcMsg )
{
	switch ( pcMsg->GetCode() )
	{
	case M_MENU_ABOUT:
		{
			Alert *about = new Alert( "About ATerm...", g_pzAboutText, Alert::ALERT_INFO,
				WND_MODAL | WND_NOT_RESIZABLE, "That's fine", NULL );

			about->CenterInWindow( GetWindow() );
			about->Go( new Invoker() );
		}
		break;
	case M_MENU_SETTINGS:
		{
			SettingsWindow *pcSettings = new SettingsWindow( Rect( 0, 0, 1, 1 ), m_pcSettings,
				"SettingsWindow",
				"Settings...", WND_MODAL | WND_NOT_RESIZABLE );

			pcSettings->CenterInWindow( GetWindow() );
			pcSettings->Show();
		}
		break;
	case M_REFRESH:
		RefreshDisplay( true );
		break;
	case M_MENU_COPY:
		Copy();
		break;
	case M_MENU_PASTE:
		Paste();
		break;
	default:
		View::HandleMessage( pcMsg );
	}
}



/*****************************************************************************
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

void TermView::MouseDown( const Point & cPosition, uint32 nButtons )
{
	bigtime_t nCurTime;
	Point nNewPosition;

	MakeFocus( true );

	switch ( nButtons )
	{
	case 1:
		if( m_bIsSelRangeValid )
		{
			RemoveSelection();
			m_bModified = true;
		}

		nCurTime = get_system_time();

		nNewPosition = cPosition - Point( 0, m_nScrollPos * m_cCharSize.y );

		if( nNewPosition == m_cSelectStart && nCurTime - m_nLastClickTime < 500000 )
		{
			/* User is performing a double click */
			m_bIsSelectingWords = true;
			if( m_bIsSelRangeValid == true )
			{
				HighlightSelection();
				m_bModified = true;
			}
		}
		else
		{
			m_bIsSelectingWords = false;
		}
		m_bIsSelecting = true;
		m_bIsSelRangeValid = true;
		m_cSelectStart = nNewPosition;
		m_cSelectEnd = m_cSelectStart;

		m_nLastClickTime = nCurTime;
		break;
	case 2:
		m_pcMenu->Open( ConvertToScreen( cPosition ) );
		m_pcMenu->SetTargetForItems( this );
		break;

	default:
		View::MouseDown( cPosition, nButtons );
		break;
	}
	return;
}

/*****************************************************************************
 *
 *   SYNOPSIS
 *
 *   FUNCTION
 *
 *   INPUTS
 *
 *   RESULT
 *
 *   SEE ALSO
 *
 ****************************************************************************/

void TermView::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	m_bIsSelecting = false;

	if( pcData == NULL )
	{
		return;
	}
	const char *pzStr;
	int i;

	for( i = 0; pcData->FindString( "file/path", &pzStr, i ) == 0; ++i )
	{

		std::string cBuffer;

		for( int j = 0; pzStr[j] != '\0'; j++ )
		{
			if( pzStr[j] == ' ' )
			{
				cBuffer += '\\';
			}
			cBuffer += pzStr[j];
		}
		cBuffer += ' ';
		write( g_nMasterPTY, cBuffer.c_str(), cBuffer.size(  ) );
	}
	if( i > 0 )
	{
		GetWindow()->MakeFocus(  );
	}
}


/**
 * Returns position of first letter of the word
 *
 * This procedure returns the position of the first letter of the word
 * relative to position \a pos.
 *
 * If position \a pos is within a word, the function returns the position
 * of the first letter of this word.
 * 
 * If char at position \a pos is a space, the function returns the
 * position of the first letter of the word on the right. In this
 * case, returned value is greater than \a pos.
 *
 */
uint TermView::GetStartBoundary( TextLine * pcLineBuf, uint pos )
{
	uint new_pos = pos;

	if( pcLineBuf == NULL )
	{
		return new_pos;
	}

	if( new_pos >= ( uint )pcLineBuf->m_nMaxLength )
	{
		new_pos = pcLineBuf->m_nMaxLength - 1;
	}

	while( !isspace( pcLineBuf->m_pCharMap[new_pos] ) )
	{
		if( new_pos == 0 )
			break;
		new_pos--;
	}
	while( isspace( pcLineBuf->m_pCharMap[new_pos] ) )
	{
		if( new_pos == ( uint )pcLineBuf->m_nMaxLength - 1 )
			break;
		new_pos++;
	};

	return new_pos;
}


/**
 * Returns position of last letter of the word
 *
 * This procedure returns the position of the last letter of the word
 * relative to position \a pos.
 *
 * If position \a pos is within a word, the function returns the position
 * of the last letter of this word.
 * 
 * If char at position \a pos is a space, the function returns the
 * position of the last letter of the word on the left. In this
 * case, returned value is smaller than \a pos.
 *
 */
uint TermView::GetEndBoundary( TextLine * pcLineBuf, uint pos )
{
	uint new_pos = pos;

	if( pcLineBuf == NULL )
	{
		return new_pos;
	}

	if( new_pos >= ( uint )pcLineBuf->m_nMaxLength )
	{
		new_pos = pcLineBuf->m_nMaxLength - 1;
	}

	while( !isspace( pcLineBuf->m_pCharMap[new_pos] ) )
	{
		if( new_pos == ( uint )pcLineBuf->m_nMaxLength - 1 )
			break;
		new_pos++;
	}
	while( isspace( pcLineBuf->m_pCharMap[new_pos] ) )
	{
		if( new_pos == 0 )
			break;
		new_pos--;
	}

	return new_pos;
}

/**
 * Compute selection positions
 *
 */
void TermView::SortSelection( IPoint * pcPnt1, IPoint * pcPnt2 )
{
	Point cPnt1;
	Point cPnt2;
	IPoint cStart;
	IPoint cEnd;


	if( PixToChar( m_cSelectStart ) < PixToChar( m_cSelectEnd ) )
	{
		cPnt1 = m_cSelectStart;
		cPnt2 = m_cSelectEnd;
		cPnt2.x -= m_cCharSize.x * 0.5f;
		if( cPnt2.x < 0 )
		{
			cPnt2.y -= m_cCharSize.y;
			cPnt2.x = 1000000.0f;
		}
	}
	else
	{
		cPnt1 = m_cSelectEnd;
		cPnt2 = m_cSelectStart;
		cPnt2.x -= m_cCharSize.x * 0.5f;
		if( cPnt2.x < 0 )
		{
			cPnt2.y -= m_cCharSize.y;
			cPnt2.x = 1000000.0f;
		}
	}
	cStart = PixToChar( cPnt1 );
	cEnd = PixToChar( cPnt2 );

	if( m_bIsSelectingWords )
	{
		TextLine *pcLineBuf;

		if( m_bIsSelRangeRect )
		{
			uint y, x;
			uint lastminx, lastmaxx;

			if( cStart.y > cEnd.y )
			{
				y = cStart.y;
				cStart.y = cEnd.y;
				cEnd.y = y;
			}
			if( cStart.x > cEnd.x )
			{
				x = cStart.x;
				cStart.x = cEnd.x;
				cEnd.x = x;
			}

			do
			{
				lastminx = cStart.x;
				lastmaxx = cEnd.x;
				for( y = cStart.y; y <= ( uint )cEnd.y; y++ )
				{
					pcLineBuf = GetLineBuf( y );

					x = GetStartBoundary( pcLineBuf, cStart.x );
					if( x < ( uint )cStart.x )
					{
						cStart.x = x;
					}
					x = GetEndBoundary( pcLineBuf, cEnd.x );
					if( x > ( uint )cEnd.x )
					{
						cEnd.x = x;
					}
				}
			}
			while( ( uint )cStart.x < lastminx || ( uint )cEnd.x > lastmaxx );
		}
		else
		{
			pcLineBuf = GetLineBuf( cStart.y );
			cStart.x = GetStartBoundary( pcLineBuf, cStart.x );

			pcLineBuf = GetLineBuf( cEnd.y );
			cEnd.x = GetEndBoundary( pcLineBuf, cEnd.x );
		}
	}

	*pcPnt1 = cStart;
	*pcPnt2 = cEnd;
}

void TermView::RemoveSelection()
{
	IPoint cPnt1;
	IPoint cPnt2;
	int nMinX;
	int nMaxX;

	SortSelection( &cPnt1, &cPnt2 );

	for( int y = cPnt1.y; y <= cPnt2.y; ++y )
	{
		TextLine *pcLineBuf = GetLineBuf( y );

		if( pcLineBuf == NULL )
		{
			continue;
		}
		if( m_bIsSelRangeRect )
		{
			nMinX = cPnt1.x;
			nMinX = cPnt2.x;

			if( cPnt1.x < cPnt2.x )
			{
				nMinX = cPnt1.x;
				nMaxX = cPnt2.x;
			}
			else
			{
				nMinX = cPnt2.x;
				nMaxX = cPnt1.x;
			}
		}
		else
		{
			if( y == cPnt1.y )
			{
				nMinX = cPnt1.x;
			}
			else
			{
				nMinX = 0;
			}
			if( y == cPnt2.y )
			{
				nMaxX = cPnt2.x;
			}
			else
			{
				nMaxX = pcLineBuf->m_nMaxLength - 1;
			}
		}

		if( nMinX < 0 )
		{
			nMinX = 0;
		}
		else if( nMinX >= pcLineBuf->m_nMaxLength )
		{
			nMinX = pcLineBuf->m_nMaxLength - 1;
		}
		if( nMaxX < 0 )
		{
			nMaxX = 0;
		}
		else if( nMaxX >= pcLineBuf->m_nMaxLength )
		{
			nMaxX = pcLineBuf->m_nMaxLength - 1;
		}

		for( int x = nMinX; x <= nMaxX; ++x )
		{
			pcLineBuf->m_pAttribMap[x] &= ~ATTR_SELECTED;
		}
		InvalidateLine( y + m_nScrollPos );
	}
}

void TermView::HighlightSelection()
{
	IPoint cPnt1;
	IPoint cPnt2;
	int nMinX;
	int nMaxX;

	SortSelection( &cPnt1, &cPnt2 );

	for( int y = cPnt1.y; y <= cPnt2.y; ++y )
	{
		TextLine *pcLineBuf = GetLineBuf( y );

		if( pcLineBuf == NULL )
		{
			continue;
		}
		if( m_bIsSelRangeRect )
		{
			nMinX = cPnt1.x;
			nMinX = cPnt2.x;

			if( cPnt1.x < cPnt2.x )
			{
				nMinX = cPnt1.x;
				nMaxX = cPnt2.x;
			}
			else
			{
				nMinX = cPnt2.x;
				nMaxX = cPnt1.x;
			}
		}
		else
		{
			if( y == cPnt1.y )
			{
				nMinX = cPnt1.x;
			}
			else
			{
				nMinX = 0;
			}
			if( y == cPnt2.y )
			{
				nMaxX = cPnt2.x;
			}
			else
			{
				nMaxX = pcLineBuf->m_nMaxLength - 1;
			}
		}

		if( nMinX < 0 )
		{
			nMinX = 0;
		}
		else if( nMinX >= pcLineBuf->m_nMaxLength )
		{
			nMinX = pcLineBuf->m_nMaxLength - 1;
		}
		if( nMaxX < 0 )
		{
			nMaxX = 0;
		}
		else if( nMaxX >= pcLineBuf->m_nMaxLength )
		{
			nMaxX = pcLineBuf->m_nMaxLength - 1;
		}

		for( int x = nMinX; x <= nMaxX; ++x )
		{
			pcLineBuf->m_pAttribMap[x] |= ATTR_SELECTED;
		}
		InvalidateLine( y + m_nScrollPos );
	}
}

void TermView::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	if( nCode == MOUSE_ENTERED )
	{
		if( m_pcSettings->GetIBeam() )
		{
			Application::GetInstance()->PushCursor( MPTR_MONO, g_anMouseImg2, POINTER2_WIDTH, POINTER2_HEIGHT, IPoint( POINTER2_WIDTH / 2, POINTER2_HEIGHT / 2 ) );
		}
		else
		{
			Application::GetInstance()->PushCursor( MPTR_MONO, g_anMouseImg1, POINTER1_WIDTH, POINTER1_HEIGHT, IPoint( POINTER1_WIDTH / 2, POINTER1_HEIGHT / 2 ) );
		}
	}
	else if( nCode == MOUSE_EXITED )
	{
		Application::GetInstance()->PopCursor(  );
	}
	if( m_bIsSelecting == false )
	{
		return;
	}
	RemoveSelection();
	if( View::GetQualifiers() & QUAL_ALT )
	{
		m_bIsSelRangeRect = true;
	}
	else
	{
		m_bIsSelRangeRect = false;
	}
	m_cSelectEnd = cNewPos - Point( 0, m_nScrollPos * m_cCharSize.y );
	HighlightSelection();
	m_bModified = true;
}

/**
 * Paint event
 *
 * This method is called by appserver when we need to refresh the window
 *
 */
void TermView::Paint( const Rect & cUpdateRect )
{
	RefreshDisplay( true );
}

/**
 * Draw the caret
 *
 * This method draws the caret over the given attribute. Depending
 * on the background color, the caret will be either white or black.
 *
 */
void TermView::RenderCursor( int nAttribs )
{
	Color32_s sBgCol = GetAttrBgColor( nAttribs );

	if( ( sBgCol.red + sBgCol.green + sBgCol.blue ) < 768 / 6 )
	{
		SetFgColor( 255, 255, 255 );
	}
	else
	{
		SetFgColor( 0, 0, 0 );
	}

	int y = m_cCsrPos.y + m_nScrollPos;

	DrawLine( Point( m_cCsrPos.x * m_cCharSize.x, y * m_cCharSize.y ), Point( m_cCsrPos.x * m_cCharSize.x, ( y + 1 ) * m_cCharSize.y - 1 ) );

	DrawLine( Point( m_cCsrPos.x * m_cCharSize.x + 1, y * m_cCharSize.y ), Point( m_cCsrPos.x * m_cCharSize.x + 1, ( y + 1 ) * m_cCharSize.y - 1 ) );
}

/**
 * Clear a line section
 *
 * This method erase a section of a text line on the screen
 *
 */
void TermView::ClearLinePart( TextLine * pcLine, int x1, int x2, int y )
{
	int16 nLastAttrib = pcLine->m_pAttribMap[x1];
	int nStartX = x1;
	int x;

	for( x = x1; x <= x2; ++x )
	{
		int16 nNextAttrib = pcLine->m_pAttribMap[x + 1];

		bool bStateChange;

		if( x != x2 )
		{
			if( ( nNextAttrib & ( ATTR_INVERSE | ATTR_SELECTED ) ) != ( nLastAttrib & ( ATTR_INVERSE | ATTR_SELECTED ) ) )
			{
				bStateChange = true;
			}
			else
			{
				bStateChange = ( nLastAttrib & ATTR_INVERSE ) ? ( nNextAttrib & ATTR_FG_MASK ) != ( nLastAttrib & ATTR_FG_MASK ) : ( nNextAttrib & ATTR_BG_MASK ) != ( nLastAttrib & ATTR_BG_MASK );
			}
		}
		else
		{
			bStateChange = true;
		}

		if( bStateChange )
		{
			if( ( nLastAttrib & ATTR_SELECTED ) == 0 )
			{
				SetFgColor( ( nLastAttrib & ATTR_INVERSE ) ? GetAttrFgColor( nLastAttrib ) : GetAttrBgColor( nLastAttrib ) );
			}
			else
			{
				SetFgColor( m_pcSettings->GetSelectedBackgroundColor() );
			}

			FillRect( Rect( nStartX * m_cCharSize.x, y * m_cCharSize.y, ( x + 1 ) * m_cCharSize.x - 1, ( y + 1 ) * m_cCharSize.y - 1 ) );
			nStartX = x + 1;
			nLastAttrib = nNextAttrib;
		}
	}
}

/**
 * Render a section of a text line
 *
 * This method will iterate through characters and do the real drawing
 * of each of them
 *
 */
void TermView::RenderLinePart( TextLine * pcLine, int x1, int x2, int y, int nAscender, bool bOnlyModified )
{
	int16 nLastAttrib;
	int nStartX = x1;
	int x;
	int nState = 0;

	ClearLinePart( pcLine, x1, x2, y );

	if( x1 > 0 )
	{
		x1--;		// Draw one extra character in case there is an overlap.
	}
	nLastAttrib = pcLine->m_pAttribMap[x1];

	nState = 1;

	for( x = x1; x <= x2; ++x )
	{
		int16 nNextAttrib = pcLine->m_pAttribMap[x + 1];

		if( nNextAttrib != nLastAttrib || x == x2 )
		{
			if( x >= nStartX )
			{
				if( ( nLastAttrib & ATTR_SELECTED ) == 0 )
				{
					SetFgColor( ( nLastAttrib & ATTR_INVERSE ) ? GetAttrBgColor( nLastAttrib ) : GetAttrFgColor( nLastAttrib ) );

					SetBgColor( ( nLastAttrib & ATTR_INVERSE ) ? GetAttrFgColor( nLastAttrib ) : GetAttrBgColor( nLastAttrib ) );
				}
				else
				{
					SetFgColor( m_pcSettings->GetSelectedTextColor() );
					SetBgColor( m_pcSettings->GetSelectedBackgroundColor() );
				}
				int nStrLen = x - nStartX + 1;
				char zBuffer[nStrLen * 3];

				const int32 *pzString = &pcLine->m_pCharMap[nStartX];

				{
					int nUTF8Len = 0;

					for( int i = 0; i < nStrLen; ++i )
					{
						int nChLen = unicode_to_utf8( zBuffer + nUTF8Len, pzString[i] );

						nUTF8Len += nChLen;
					}
					nStrLen = nUTF8Len;
				}

				MovePenTo( nStartX * m_cCharSize.x, y * m_cCharSize.y + m_sFontHeight.ascender + m_sFontHeight.line_gap );
				DrawString( zBuffer, nStrLen );

				if( nLastAttrib & ATTR_BOLD )
				{
					MovePenTo( nStartX * m_cCharSize.x + 1, y * m_cCharSize.y + m_sFontHeight.ascender + m_sFontHeight.line_gap );
					SetDrawingMode( DM_OVER );
					DrawString( zBuffer, nStrLen );
					SetDrawingMode( DM_COPY );
				}
				if( nLastAttrib & ATTR_UNDERLINE )
				{
					DrawLine( Point( nStartX * m_cCharSize.x, ( y + 1 ) * m_cCharSize.y - 1 ), Point( ( nStartX + x - nStartX + 1 ) * m_cCharSize.x, ( y + 1 ) * m_cCharSize.y - 1 ) );
				}
			}
			nLastAttrib = nNextAttrib;
			nStartX = x + 1;
		}
	}

}

/**
 * Render a whole line
 *
 * This method calls RenderLinePart to get the real drawing done.
 *
 */
void TermView::RenderLine( int y, int nAscender, bool bOnlyModified )
{
	int x;
	int x1 = 0;
	int nState = 0;
	TextLine *pcLine = GetLineBuf( y, true );
	TextLine *pcPrevLine = GetPrevLineBuf( y );

	if( NULL == pcLine )
	{
		dbprintf( "RenderLine(%d, %d, %d) failed to get line buffer\n", y, nAscender, bOnlyModified );
		return;
	}
	if( NULL == pcPrevLine )
	{
		dbprintf( "RenderLine(%d, %d, %d) failed to get prev line buffer\n", y, nAscender, bOnlyModified );
		return;
	}

	if( false == bOnlyModified )
	{
		RenderLinePart( pcLine, 0, m_cCurCharMapSize.x - 1, y, nAscender, false );
	}
	else
	{
		for( x = 0; x <= m_cCurCharMapSize.x; ++x )
		{
			switch ( nState )
			{
			case 0:
				if( pcLine->m_pCharMap[x] != pcPrevLine->m_pCharMap[x] || pcLine->m_pAttribMap[x] != pcPrevLine->m_pAttribMap[x] )
				{
					x1 = x;
					nState = 1;
				}
				break;
			case 1:
				if( ( pcLine->m_pCharMap[x] == pcPrevLine->m_pCharMap[x] && pcLine->m_pAttribMap[x] == pcPrevLine->m_pAttribMap[x] ) || x == m_cCurCharMapSize.x - 1 )
				{
					RenderLinePart( pcLine, x1, x - ( ( x == m_cCurCharMapSize.x - 1 ) ? 0 : 1 ), y, nAscender, false );
					nState = 0;
				}
				break;
			}
			pcPrevLine->m_pCharMap[x] = pcLine->m_pCharMap[x];
			pcPrevLine->m_pAttribMap[x] = pcLine->m_pAttribMap[x];
		}
	}
	if( y == m_cCsrPos.y + m_nScrollPos )
	{
		RenderCursor( pcLine->m_pAttribMap[m_cCsrPos.x] );
	}
}

/**
 * Redraw part or all display
 *
 * This method calls RenderLine
 *
 */
void TermView::RefreshDisplay( bool bAll )
{
	int y;

	if( NULL == m_pBuffer )
	{
		dbprintf( "Refresh called with m_pBuffer == NULL!!\n" );
		return;
	}

	if( m_bModified || bAll )
	{
		Rect cBounds = GetBounds();

		if( m_cCurCharMapSize.x * m_cCharSize.x < cBounds.Width() + 1.0f )
		{
			SetFgColor( GetAttrBgColor( m_nCurAttrib ) );

			FillRect( Rect( m_cCurCharMapSize.x * m_cCharSize.x, 0, 1000000, 1000000 ) );
		}

		if( m_cCurCharMapSize.y * m_cCharSize.y < cBounds.Height() + 1.0f )
		{
			SetFgColor( GetAttrBgColor( m_nCurAttrib ) );

			FillRect( Rect( 0, m_cCurCharMapSize.y * m_cCharSize.y, 1000000, 1000000 ) );
		}

		if( m_cLastCsrPos != IPoint( m_cCsrPos.x, m_cCsrPos.y + m_nScrollPos ) )
		{
			// Force rendering of char below old csr
			if( m_cLastCsrPos.x >= 0 && m_cLastCsrPos.x < m_cCurCharMapSize.x && m_cLastCsrPos.y >= 0 && m_cLastCsrPos.y < m_cCurCharMapSize.y )
			{
				GetPrevLineBuf( m_cLastCsrPos.y )->m_pCharMap[m_cLastCsrPos.x] = -1;
			}
			m_cLastCsrPos = IPoint( m_cCsrPos.x, m_cCsrPos.y + m_nScrollPos );
		}

		if( g_bDontRender )
		{
			dbprintf( "PANIC!!!!!!!!!!!!! RefreshDisplay() Called while g_bDontRender was true!!!!!!!!!!!!!!!\n" );
			return;
		}

		Font *pcFont = GetFont();

		if( NULL != pcFont )
		{
			for( y = 0; y < m_cCurCharMapSize.y; ++y )
			{
				if( bAll || m_pabModifiedLines[y] )
				{
					RenderLine( y, ( int )m_sFontHeight.ascender, !bAll );
					// Preserve the dirty flag if from Paint() since the rendering might get clipped away
					m_pabModifiedLines[y] = bAll;
				}
			}
			Flush();
		}
		m_bModified = bAll;	// Preserve the dirty flag if from pain() since the rendering might get clipped
	}
}


/**
 *
 */
void TermView::Scroll( int nDeltaY, int y1, int y2 )
{
	int y;

	if( nDeltaY < 0 )
	{
		int nSteps = -nDeltaY;

		if( 0 == m_nCurrentBuffer && 0 == y1 && y2 == m_cCurCharMapSize.y - 1 )
		{
			if( m_nTotLineCnt + nSteps > m_nMaxLineCount )
			{
				if( m_nTotLineCnt < 1500 )
				{
					m_nMaxLineCount = m_nTotLineCnt + nSteps + 100;

					TextLine **pNewBuf = new TextLine *[m_nMaxLineCount];

					memcpy( pNewBuf + nSteps, m_pBuffer, sizeof( *m_pBuffer ) * m_nTotLineCnt );

					delete[]m_pBuffer;
					m_pBuffer = pNewBuf;

					m_nTotLineCnt += nSteps;
				}
				else
				{
					int i;

					for( i = m_nTotLineCnt; i < m_nMaxLineCount; ++i )
					{
						m_pBuffer[i] = new TextLine( ' ', m_nCurAttrib );
					}

					for( i = 0; i < nSteps; ++i )
					{
						TextLine *pcTmp = m_pBuffer[m_nMaxLineCount - 1];

						memmove( m_pBuffer + 1, m_pBuffer, sizeof( *m_pBuffer ) * ( m_nMaxLineCount - 1 ) );
						m_pBuffer[0] = pcTmp;
						pcTmp->Clear( 0, TextLine::MAX, ' ', m_nCurAttrib );
					}
					nSteps = 0;	// Dont alloc any new lines
				}
			}
			else
			{
				memmove( m_pBuffer + nSteps, m_pBuffer, sizeof( *m_pBuffer ) * m_nTotLineCnt );

				m_nTotLineCnt += nSteps;
			}

			if( NULL != g_pcScrollBar )
			{
				g_pcScrollBar->SetProportion( float ( m_cCurCharMapSize.y ) / float ( m_nTotLineCnt ) );

				g_pcScrollBar->SetMinMax( 0, m_nTotLineCnt - m_cCurCharMapSize.y );
				g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );
			}
			int i;

			for( i = 0; i < nSteps; ++i )
			{
				m_pBuffer[i] = new TextLine( ' ', m_nCurAttrib );
			}
		}
		else
		{
			// Scroll region

			if( y2 > y1 )
			{
				int nBottomOffset = m_cCurCharMapSize.y - y2 - 1;
				int i;

				for( i = 0; i < nSteps; ++i )
				{
					TextLine *pcTmp = GetLineBuf( y1 );

					memmove( m_pBuffer + 1 + nBottomOffset, m_pBuffer + nBottomOffset, sizeof( *m_pBuffer ) * ( y2 - y1 ) );
					/*m_pBuffer[ y1 ] */ m_pBuffer[m_cCurCharMapSize.y - y2 -
						1] = pcTmp;
					pcTmp->Clear( 0, TextLine::MAX, ' ', m_nCurAttrib );
				}
			}
		}

		for( y = y1; y <= y2; ++y )
		{
			InvalidateLine( y );
		}
		m_bModified = true;
	}
	else
	{
		int nSteps = nDeltaY;

		if( y2 > y1 )
		{
			int nBottomOffset = m_cCurCharMapSize.y - y2 - 1;
			int i;

			for( i = 0; i < nSteps; ++i )
			{
				TextLine *pcTmp = GetLineBuf( y2, false );

				memmove( m_pBuffer + nBottomOffset, m_pBuffer + nBottomOffset + 1, sizeof( *m_pBuffer ) * ( y2 - y1 ) );
				m_pBuffer[m_cCurCharMapSize.y - y1 - 1] = pcTmp;
				pcTmp->Clear( 0, TextLine::MAX, ' ', m_nCurAttrib );
			}
			for( y = y1; y <= y2; ++y )
			{
				InvalidateLine( y );
			}
		}
		m_bModified = true;
	}
	m_cCsrPos.y += nDeltaY;
	if( m_bIsSelRangeValid )
	{
		m_cSelectStart.y += nDeltaY * m_cCharSize.y;
		m_cSelectEnd.y += nDeltaY * m_cCharSize.y;
	}
}

/**
 *
 */
void TermView::MoveCsr( int x, int y, bool bScroll )
{
	SetCsr( m_cCsrPos.x + x, m_cCsrPos.y + y, bScroll );
}

/**
 *
 */
void TermView::SetCsr( int x, int y, bool bScroll )
{
	InvalidateLine( m_cCsrPos.y );
	m_cCsrPos.x = x;
	m_cCsrPos.y = y;
	InvalidateLine( m_cCsrPos.y );


	if( bScroll )
	{
		if( m_cCsrPos.y > m_nScrollBottom )
		{
			Scroll( m_nScrollBottom - m_cCsrPos.y, m_nScrollTop, m_nScrollBottom );
		}
		if( m_cCsrPos.y < m_nScrollTop )
		{
			Scroll( m_nScrollTop - m_cCsrPos.y, m_nScrollTop, m_nScrollBottom );
		}
	}
	if( m_cCsrPos.x < 0 )
		m_cCsrPos.x = 0;
	if( m_cCsrPos.x >= m_cCurCharMapSize.x )
		m_cCsrPos.x = m_cCurCharMapSize.x - 1;

	if( m_cCsrPos.y < 0 )
		m_cCsrPos.y = 0;
	if( m_cCsrPos.y >= m_cCurCharMapSize.y )
		m_cCsrPos.y = m_cCurCharMapSize.y - 1;

	m_bModified = true;
}

/**
 *
 */
void TermView::PutChar( int32 nChar )
{
	if( m_nCharSet == 0 || m_nCharSet == -1 )
	{
		if( nChar < 32 && m_nCharSet != -1 )
		{
			switch ( nChar )
			{
			case 0x07:	/* BELL */
				break;
			case 0x08:	/* BACKSPACE    */
				MoveCsr( -1, 0, false );
				break;
			case '\n':
				MoveCsr( 0, 1, true );
				break;
			case '\r':
				SetCsr( 0, m_cCsrPos.y, true );
				break;
			case '\t':
				if( ( m_cCsrPos.x % 8 ) == 0 )
				{
					PutChar( ' ' );
				}
				while( m_cCsrPos.x % 8 )
				{
					PutChar( ' ' );
				}
				break;

			case 14:
				m_nCharSet = 1;
			case 15:
				// mc sends to many to bother prints them. Dont know what they are for yet :(
				break;
			default:
				DEBUG( 2, "Unknown control code %ld\n", nChar );
				break;
			}
		}
		else
		{
			if( m_cCsrPos.x >= m_cCurCharMapSize.x )
			{
				SetCsr( 0, m_cCsrPos.y + 1, true );
			}

			if( m_cCsrPos.x < 0 || m_cCsrPos.x >= m_cCurCharMapSize.x || m_cCsrPos.y < 0 || m_cCsrPos.y >= m_cCurCharMapSize.y )
			{
				return;
			}
			TextLine *pcLineBuf = GetLineBuf( m_cCsrPos.y );

			if( m_bInsertMode && m_cCsrPos.x < pcLineBuf->m_nMaxLength - 1 )
			{
				memmove( pcLineBuf->m_pCharMap + m_cCsrPos.x + 1, pcLineBuf->m_pCharMap + m_cCsrPos.x, ( pcLineBuf->m_nMaxLength - m_cCsrPos.x - 1 ) * sizeof( int32 ) );
				memmove( pcLineBuf->m_pAttribMap + m_cCsrPos.x + 1, pcLineBuf->m_pAttribMap + m_cCsrPos.x, ( pcLineBuf->m_nMaxLength - m_cCsrPos.x - 1 ) * sizeof( int16 ) );
			}
			pcLineBuf->m_pCharMap[m_cCsrPos.x] = nChar;
			pcLineBuf->m_pAttribMap[m_cCsrPos.x] = m_nCurAttrib;
			InvalidateLine( m_cCsrPos.y );
			m_cCsrPos.x++;
			m_bModified = true;
		}
	}
	else
	{
		int nOldAttrib = m_nCurAttrib;

		switch ( nChar )
		{
		case 0x07:	/* BELL */
			break;
		case 0x08:	/* BACKSPACE    */
			MoveCsr( -1, 0, false );
			break;
		case '\n':
			MoveCsr( 0, 1, true );
			break;
		case '\r':
			SetCsr( 0, m_cCsrPos.y, true );
			break;
		case '\t':
			if( ( m_cCsrPos.x % 8 ) == 0 )
			{
				PutChar( ' ' );
			}
			while( m_cCsrPos.x % 8 )
			{
				PutChar( ' ' );
			}
			break;

		case 'j':
			m_nCharSet = -1;
			PutChar( 0x18 );
			m_nCharSet = 1;
			break;	// 0x2518 (RB)
		case 'k':
			m_nCharSet = -1;
			PutChar( 0x10 );
			m_nCharSet = 1;
			break;	// 0x2510 (RT)
		case 'l':
			m_nCharSet = -1;
			PutChar( 0x0c );
			m_nCharSet = 1;
			break;	// 0x250c (LT)
		case 'm':
			m_nCharSet = -1;
			PutChar( 0x14 );
			m_nCharSet = 1;
			break;	// 0x2514 (LB)
		case 't':
			m_nCharSet = -1;
			PutChar( 0x1c );
			m_nCharSet = 1;
			break;	// 0x251c (|-)
		case 'u':
			m_nCharSet = -1;
			PutChar( 0x24 );
			m_nCharSet = 1;
			break;	// 0x2524 (-|)
		case 'q':
			m_nCharSet = -1;
			PutChar( 0x00 );
			m_nCharSet = 1;
			break;	// 0x2500 (-)
		case 'x':
			m_nCharSet = -1;
			PutChar( 0x02 );
			m_nCharSet = 1;
			break;	// 0x2502 (|)

/*
	    case 'j':	m_nCharSet = 0; PutChar( 188 ); m_nCharSet = 1; break; // 0x255d
	    case 'k':	m_nCharSet = 0; PutChar( 187 ); m_nCharSet = 1; break; // 0x2557
	    case 'l':	m_nCharSet = 0; PutChar( 201 ); m_nCharSet = 1; break; // 0x2554
	    case 'm':	m_nCharSet = 0; PutChar( 200 ); m_nCharSet = 1; break; // 0x255a
	    case 't':	m_nCharSet = 0; PutChar( 204 ); m_nCharSet = 1; break; // 0xs560
	    case 'u':	m_nCharSet = 0; PutChar( 202 ); m_nCharSet = 1; break; // 0x2569
	    case 'q':	m_nCharSet = 0; PutChar( 205 ); m_nCharSet = 1; break; // 0x2550
	    case 'x':	m_nCharSet = 0; PutChar( 186 ); m_nCharSet = 1; break; // 0x2551
	    */
		case 14:
			m_nCharSet = 1;
			break;
		case 15:
			m_nCharSet = 0;
			break;
		default:
			m_nCharSet = -1;
			PutChar( nChar );
			m_nCharSet = 1;
			break;
//              printf( "Unknown graphics char %d\n", nChar );
			break;
		}
		m_nCurAttrib = nOldAttrib;
	}
}

/**
 *
 */
void TermView::EraseToEndOfLine()
{
	int i;

	if( m_cCsrPos.x < 0 || m_cCsrPos.x >= m_cCurCharMapSize.x || m_cCsrPos.y < 0 || m_cCsrPos.y >= m_cCurCharMapSize.y )
	{
		return;
	}
	for( i = ( m_cCsrPos.x >= 0 ) ? m_cCsrPos.x : 0; i < m_cCurCharMapSize.x; ++i )
	{
		GetLineBuf( m_cCsrPos.y )->m_pCharMap[i] = ' ';
		GetLineBuf( m_cCsrPos.y )->m_pAttribMap[i] = m_nCurAttrib;
	}
	InvalidateLine( m_cCsrPos.y );
	m_bModified = true;
}

/**
 *
 */
void TermView::EraseToCursor()
{
	int i;

	if( m_cCsrPos.x < 0 || m_cCsrPos.x >= m_cCurCharMapSize.x || m_cCsrPos.y < 0 || m_cCsrPos.y >= m_cCurCharMapSize.y )
	{
		return;
	}

	for( i = 0; i < m_cCsrPos.x; ++i )
	{
		GetLineBuf( m_cCsrPos.y )->m_pCharMap[i] = ' ';
		GetLineBuf( m_cCsrPos.y )->m_pAttribMap[i] = m_nCurAttrib;
	}
	InvalidateLine( m_cCsrPos.y );
	m_bModified = true;
}

/**
 *
 */
void TermView::EraseLine()
{
	int i;

	if( m_cCsrPos.y < 0 || m_cCsrPos.y >= m_cCurCharMapSize.y )
	{
		return;
	}

	for( i = 0; i < m_cCurCharMapSize.x; ++i )
	{
		GetLineBuf( m_cCsrPos.y )->m_pCharMap[i] = ' ';
		GetLineBuf( m_cCsrPos.y )->m_pAttribMap[i] = m_nCurAttrib;
	}
	InvalidateLine( m_cCsrPos.y );
	m_bModified = true;
}

/**
 *
 */
void TermView::EraseFromTopToCursor()
{
	int i;

	if( m_cCsrPos.y < 0 || m_cCsrPos.y >= m_cCurCharMapSize.y )
	{
		return;
	}

	for( i = 0; i < m_cCsrPos.y; ++i )
	{
		GetLineBuf( i )->Clear( 0, m_cCurCharMapSize.x, ' ', m_nCurAttrib );
		InvalidateLine( i );
	}
	m_bModified = true;
}

/**
 *
 */
void TermView::EraseToBottom()
{
	int i;

	if( m_cCsrPos.y < 0 || m_cCsrPos.y >= m_cCurCharMapSize.y )
	{
		return;
	}

	for( i = m_cCsrPos.y + 1; i < m_cCurCharMapSize.y; ++i )
	{
		GetLineBuf( i )->Clear( 0, m_cCurCharMapSize.x, ' ', m_nCurAttrib );
		InvalidateLine( i );
	}
	m_bModified = true;
}

/**
 *
 */
void TermView::EraseScreen()
{
	int i;

	for( i = 0; i < m_cCurCharMapSize.y; ++i )
	{
		GetLineBuf( i )->Clear( 0, m_cCurCharMapSize.x, ' ', m_nCurAttrib );
		InvalidateLine( i );
	}
	m_bModified = true;
}

/**
 *
 */
void TermView::DeleteChars( int count )
{
	if( count == 0 )
	{
		return;
	}

	if( m_cCsrPos.x < 0 || m_cCsrPos.x >= m_cCurCharMapSize.x || m_cCsrPos.y < 0 || m_cCsrPos.y >= m_cCurCharMapSize.y )
	{
		return;
	}

	if( count + m_cCsrPos.x > m_cCurCharMapSize.x )
	{
		count = m_cCurCharMapSize.x - m_cCsrPos.x;
	}

	int len = ( m_cCurCharMapSize.x - m_cCsrPos.x ) - count;

	if( len > 0 )
	{
		memmove( &GetLineBuf( m_cCsrPos.y )->m_pCharMap[m_cCsrPos.x], &GetLineBuf( m_cCsrPos.y )->m_pCharMap[m_cCsrPos.x + count], len * sizeof( int32 ) );

		memmove( &GetLineBuf( m_cCsrPos.y )->m_pAttribMap[m_cCsrPos.x], &GetLineBuf( m_cCsrPos.y )->m_pAttribMap[m_cCsrPos.x + count], len * sizeof( int16 ) );

	}

	GetLineBuf( m_cCsrPos.y )->Clear( m_cCurCharMapSize.x - count, TextLine::MAX, ' ', m_nCurAttrib );

/*    memset( &GetLineBuf( m_cCsrPos.y )->m_pCharMap[m_cCurCharMapSize.x -
                count], ' ', count * sizeof( int32 ) );

    int i;

    for( i = 0; i < count; ++i ) {
        GetLineBuf( m_cCsrPos.y )->m_pAttribMap[( m_cCurCharMapSize.x -
                    count ) + i] = m_nCurAttrib;
    }*/

	InvalidateLine( m_cCsrPos.y );
	m_bModified = true;
}

/**
 *
 */
void TermView::InsertChars( int count )
{
	if( count == 0 )
	{
		return;
	}

	if( m_cCsrPos.x < 0 || m_cCsrPos.x >= m_cCurCharMapSize.x || m_cCsrPos.y < 0 || m_cCsrPos.y >= m_cCurCharMapSize.y )
	{
		return;
	}

	if( count + m_cCsrPos.x > m_cCurCharMapSize.x )
	{
		count = m_cCurCharMapSize.x - m_cCsrPos.x;
	}

	int len = ( m_cCurCharMapSize.x - m_cCsrPos.x ) - count;

	if( len > 0 )
	{
		memmove( &GetLineBuf( m_cCsrPos.y )->m_pCharMap[m_cCsrPos.x + count], &GetLineBuf( m_cCsrPos.y )->m_pCharMap[m_cCsrPos.x], len * sizeof( int32 ) );

		memmove( &GetLineBuf( m_cCsrPos.y )->m_pAttribMap[m_cCsrPos.x + count], &GetLineBuf( m_cCsrPos.y )->m_pAttribMap[m_cCsrPos.x], len * sizeof( int16 ) );

	}

	GetLineBuf( m_cCsrPos.y )->Clear( m_cCsrPos.x, m_cCsrPos.x + count - 1, ' ', m_nCurAttrib );

	InvalidateLine( m_cCsrPos.y );
	m_bModified = true;
}

void TermView::DeleteLines( int nCount )
{
	Scroll( -nCount, m_cCsrPos.y, m_nScrollBottom );
}

void TermView::InsertLines( int nCount )
{
	Scroll( nCount, m_cCsrPos.y, m_nScrollBottom );
}

/**
 *
 */
void TermView::InvalideEsc()
{
	for( int i = 0; i < m_nEscStringSize; ++i )
	{
		if( m_zEscString[i] == 0x1b )
			m_zEscString[i] = '?';
	}
	m_zEscString[m_nEscStringSize] = 0;
	DEBUG( 1, "Unrecognized escape sequens \"<ESC>%s\"\n", &m_zEscString[1] );
}

/**
 *
 */
void TermView::PrintEscStr()
{
	char zStr[514];

	if( g_nDebugLevel < 4 || m_nEscStringSize == 0 )
	{
		return;
	}

	int i;

	for( i = 1; i < m_nEscStringSize; ++i )
	{
		if( m_zEscString[i] == 0x1b )
		{
			zStr[i - 1] = '?';
		}
		else
		{
			zStr[i - 1] = m_zEscString[i];
		}
	}
	zStr[i - 1] = '\0';

	printf( "\"<ESC>%s\"\n", zStr );
}

/**
 *
 */
void TermView::ParseBraceEsc()
{
	int argv[16];
	int argc = 0;
	int nNumStart = 2;
	int nDefault = 1;
	int i;

	m_zEscString[m_nEscStringSize] = 0;

	nNumStart = ( '?' == m_zEscString[2] ) ? 3 : 2;

	for( i = nNumStart; i < m_nEscStringSize; ++i )
	{
		if( m_zEscString[i] == ';' || isdigit( m_zEscString[i] ) == false )
		{
			if( isdigit( m_zEscString[nNumStart] ) )
			{
				argv[argc++] = atoi( &m_zEscString[nNumStart] );
				nNumStart = i + 1;
			}
		}
	}

	if( argc > 0 )
	{
		nDefault = argv[0];
	}

	if( '?' == m_zEscString[2] )
	{
		switch ( m_zEscString[m_nEscStringSize - 1] )
		{
		case 'h':	// Set
			if( 1 == argc )
			{
				switch ( argv[0] )
				{
				case 1:
					m_bAlternateCursorKeys = true;
					break;
				case 5:
					m_nCurAttrib |= ATTR_INVERSE;
					break;
				case 47:
					DEBUG( 2, ( "Buffer 1 enabled\n" ) );
					m_nCurrentBuffer = 1;
					break;
				default:
					InvalideEsc();
					m_nEscStringSize = 0;
					break;
				}
			}
			else
			{
				InvalideEsc();
				m_nEscStringSize = 0;
			}
			break;
		case 'l':	// Unset
			if( 1 == argc )
			{
				switch ( argv[0] )
				{
				case 1:
					m_bAlternateCursorKeys = false;
					break;
				case 5:
					m_nCurAttrib &= ~ATTR_INVERSE;
					break;
				case 47:
					m_nCurrentBuffer = 0;
					DEBUG( 2, ( "Buffer 0 enabled\n" ) );
					break;
				default:
					InvalideEsc();
					m_nEscStringSize = 0;
					break;
				}
			}
			break;
		}
		DEBUG( 3, "argc = %d, argv[0] = %d\n", argc, argv[0] );
	}
	else
	{
		switch ( m_zEscString[m_nEscStringSize - 1] )
		{
		case 'A':	// Move csr Up, no scroll.
			MoveCsr( 0, -nDefault, false );
			break;
		case 'B':	// Move csr Down, no scroll.
			MoveCsr( 0, nDefault, false );
			break;
		case 'C':	// Move csr Right n columns.
			MoveCsr( nDefault, 0, false );
			break;
		case 'D':	// Move csr Left n columns.
			MoveCsr( -nDefault, 0, false );
			break;
		case 'E':	// CR and down, no scroll.
			SetCsr( 0, m_cCsrPos.y + nDefault, false );
//              PutChar( '\r' );    // FIX ME FIX ME FIX ME Check if this will scroll
//              MoveCsr( 0, nDefault );
			break;
		case 'F':	// CR and up, no scroll.
			SetCsr( 0, m_cCsrPos.y - nDefault, false );
//              PutChar( '\r' );
//              MoveCsr( 0, -nDefault );
			break;
		case 'G':	// CR
			PutChar( '\r' );
			break;
		case 'H':	// Move to row r, column c.
			if( 0 == argc )
			{
				SetCsr( 0, 0, false );
			}
			else if( 1 == argc )
			{
				SetCsr( 0, argv[0] - 1, false );
			}
			else if( 2 == argc )
			{
				SetCsr( argv[1] - 1, argv[0] - 1, false );
			}
			else
			{
				dbprintf( "ESC[r;nH sendt width %d numbers!\n", argc );
			}
			break;
		case 'I':	// Tab
			PutChar( '\t' );
			break;
		case 'd':	// Move to row
			SetCsr( 0, argc > 0 ? argv[0] - 1 : 0, false );
			break;
		case 'J':
			if( 0 == argc || 0 == argv[0] )
			{
				EraseToEndOfLine();
				EraseToBottom();
			}
			if( 1 == argc )
			{
				if( 1 == argv[0] )
				{
					EraseFromTopToCursor();
				}
				if( 2 == argv[0] )
				{
					EraseScreen();
					SetCsr( 0, 0, false );
				}
			}
			break;
		case 'K':
			if( 0 == argc || 0 == argv[0] )
			{
				EraseToEndOfLine();
			}
			if( 1 == argc )
			{
				switch ( argv[0] )
				{
				case 1:
					EraseToCursor();
					break;
				case 2:
					EraseLine();
					break;
				default:
					InvalideEsc();
					m_nEscStringSize = 0;
				}
			}
			break;
		case 'L':	// Inserts a blank line at cursor.
			InsertLines( ( argc > 0 ) ? argv[0] : 1 );
			break;
		case 'P':
			DeleteChars( ( 0 == argc ) ? 1 : argv[0] );
			break;
		case '@':
			InsertChars( ( 0 == argc ) ? 1 : argv[0] );
			break;
		case '`':	// Moves cursor to column n.
			SetCsr( nDefault, m_cCsrPos.y, false );
			break;
		case 'm':	// Set display characteristics
			m_nCurAttrib = g_nDefaultAttribs;
			for( i = 0; i < argc; ++i )
			{
				switch ( argv[i] )
				{
				case 0:
					m_nCurAttrib = g_nDefaultAttribs;
					break;
				case 1:
					m_nCurAttrib |= ATTR_BOLD;
					break;
				case 2:
					m_nCurAttrib |= ATTR_DIM;
					break;
				case 4:
					m_nCurAttrib |= ATTR_UNDERLINE;
					break;
				case 5:
					m_nCurAttrib |= ATTR_BLINK;
					break;
				case 7:
					m_nCurAttrib |= ATTR_INVERSE;
					break;
				case 8:
					m_nCurAttrib |= ATTR_INVISIBLE;
					break;
				default:
					if( argv[i] >= 30 && argv[i] <= 37 )
					{
						m_nCurAttrib = ( m_nCurAttrib & ~ATTR_FG_MASK ) | argv[i] - 30;
					}
					if( argv[i] >= 40 && argv[i] <= 47 )
					{
						m_nCurAttrib = ( m_nCurAttrib & ~ATTR_BG_MASK ) | ( ( argv[i] - 40 ) << 4 );
					}
					break;
				}
			}
			break;
		case 'j':
			InvalideEsc();
			m_nEscStringSize = 0;
			break;
		case 'k':
			InvalideEsc();
			m_nEscStringSize = 0;
			break;
		case 'h':
			if( 1 == argc )
			{
				switch ( argv[0] )
				{
				case 4:	// Insert mode
					m_bInsertMode = true;
					break;
				default:
					InvalideEsc();
					m_nEscStringSize = 0;
					break;
				}
			}
			else
			{
				InvalideEsc();
				m_nEscStringSize = 0;
			}
			break;
		case 'l':
			if( 1 == argc )
			{
				switch ( argv[0] )
				{
				case 4:	// Replace mode
					m_bInsertMode = false;
					// Always in replace mode ( see "[4h" )
					break;
				default:
					InvalideEsc();
					m_nEscStringSize = 0;
					break;
				}
			}
			else
			{
				InvalideEsc();
				m_nEscStringSize = 0;
			}
			break;
		case 'M':
			DeleteLines( ( argc > 0 ) ? argv[0] : 1 );
			break;
		case 'r':	// Define scrolling region
			if( argc < 2 || argv[0] >= argv[1] )
				SetScrollWindow( 0, 1000000 );
			else
				SetScrollWindow( argv[0] - 1, argv[1] - 1 );
			break;
		default:
			InvalideEsc();
			m_nEscStringSize = 0;
			break;
		}
	}
}

/**
 *
 */
void TermView::ParseParanEsc()
{
	int argv[16];
	int argc = 0;
	int nNumStart = 2;
	int i;

	m_zEscString[m_nEscStringSize] = 0;

	for( i = 2; i < m_nEscStringSize; ++i )
	{
		if( m_zEscString[i] == ';' || isdigit( m_zEscString[i] ) == false )
		{
			if( isdigit( m_zEscString[nNumStart] ) )
			{
				argv[argc++] = atoi( &m_zEscString[nNumStart] );
				nNumStart = i + 1;
			}
		}
	}
	switch ( m_zEscString[m_nEscStringSize - 1] )
	{
	case 'A':		// British 
	case 'B':		// North American ASCII set
	case 'C':		// Finnish
	case 'E':		// Danish or Norwegian
	case 'H':		// Swedish
	case 'K':		// German
	case 'Q':		// French Canadian
	case 'R':		// Flemish or French/Belgian
	case 'Y':		// Italian
	case 'Z':		// Spanish
	case '0':		// Line Drawing
	case '1':		// Alternative Character
	case '2':		// Alternative Line drawing
	case '4':		// Dutch
	case '5':		// Finnish
	case '6':		// Danish or Norwegian
	case '7':		// Swedish
	case '=':		// Swiss (French or German)
		break;
	default:
		InvalideEsc();
		m_nEscStringSize = 0;
		break;
	}

}

/**
 *
 */
void TermView::AddEscCode( char nChar )
{
	m_zEscString[m_nEscStringSize] = nChar;
	m_nEscStringSize++;

	if( '\x1b' == nChar )
	{
		return;
	}

	if( 2 == m_nEscStringSize )
	{
		switch ( nChar )
		{
		case 'c':	// Resets terminal.
			Reset();
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case 'D':	// Down, scroll in region.
			MoveCsr( 0, 1, true );
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case 'E':	// CR and cursor down (with scroll)
			SetCsr( 0, m_cCsrPos.y + 1, true );
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case 'M':	// Up, scroll in region (see later).
			MoveCsr( 0, -1, true );
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case '7':	// Save position and attributes.
			m_cSavedCsrPos = m_cCsrPos;
			m_nSavedAttribs = m_nCurAttrib;
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case '8':	// Restore position and attributes.
			m_cCsrPos = m_cSavedCsrPos;
			m_nCurAttrib = m_nSavedAttribs;
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case '<':
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case '>':
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case '[':
		case ']':
		case '(':
			break;
		case ')':
			PrintEscStr();
			m_nEscStringSize = 0;
			break;

		default:
			InvalideEsc();
			m_nEscStringSize = 0;
		}
	}
	else
	{
		switch ( m_zEscString[1] )
		{
		case '[':
			if( !( isdigit( nChar ) || nChar == ';' || nChar == '?' ) )
			{
				ParseBraceEsc();
				PrintEscStr();
				m_nEscStringSize = 0;
			}
			break;
		case ']':
			{
				static char zWindowTitle[512];
				static char zPrevWindowTitle[512];
				static int nTitleLen = 0;

				if( m_nEscStringSize == 3 )
				{
					if( nChar != '0' )
					{
						InvalideEsc();
						m_nEscStringSize = 0;
					}
					break;
				}
				if( m_nEscStringSize == 4 )
				{
					if( nChar != ';' )
					{
						InvalideEsc();
						m_nEscStringSize = 0;
					}
					break;
				}
				if( nChar != 7 )
				{
					if( nTitleLen < 512 - 1 )
					{
						zWindowTitle[nTitleLen++] = nChar;
					}
					else
					{
						zWindowTitle[511] = '\0';
					}
				}
				else
				{
					Window *pcWnd = GetWindow();

					zWindowTitle[nTitleLen++] = '\0';

					if( pcWnd != NULL && strcmp( zWindowTitle, zPrevWindowTitle ) != 0 )
					{
						pcWnd->SetTitle( zWindowTitle );
						strcpy( zPrevWindowTitle, zWindowTitle );
					}
					nTitleLen = 0;
					PrintEscStr();
					m_nEscStringSize = 0;
				}
				break;
			}
		case '(':
			if( !( isdigit( nChar ) || nChar == ';' || nChar == '?' ) )
			{
				ParseParanEsc();
				PrintEscStr();
				m_nEscStringSize = 0;
			}
			break;
		case '>':
			m_nKeyPadMode = 0;
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		case '=':
			m_nKeyPadMode = 1;
			PrintEscStr();
			m_nEscStringSize = 0;
			break;
		default:
			InvalideEsc();
			m_nEscStringSize = 0;
		}
	}

	if( m_nEscStringSize > 512 )
	{
		InvalideEsc();
		m_nEscStringSize = 0;
	}
}

/**
 *
 */
int TermView::Write( const char *zBuf, int nSize )
{
	for( int i = 0; i < nSize; )
	{
		DEBUG( 6, "%03d '%c'\n", zBuf[i], ( zBuf[i] < 32 ) ? '.' : zBuf[i] );
		int nCharLen = utf8_char_length( zBuf[i] );

		if( m_nEscStringSize > 0 || '\x1b' == zBuf[i] )
		{
			AddEscCode( uint8 ( zBuf[i] ) );
		}
		else
		{
			PutChar( utf8_to_unicode( &zBuf[i] ) );
		}
		i += nCharLen;
	}
	return ( 0 );
}

/**
 *
 */
void TermView::ScrollBack( int nPos )
{
	m_nScrollPos = m_nTotLineCnt - nPos - m_cCurCharMapSize.y;

	if( m_nScrollPos < 0 )
	{
		m_nScrollPos = 0;
	}
	if( m_nScrollPos >= m_nTotLineCnt - m_cCurCharMapSize.y )
	{
		m_nScrollPos = m_nTotLineCnt - m_cCurCharMapSize.y;	// - 1;
	}
	memset( m_pabModifiedLines, true, sizeof( m_pabModifiedLines[0] ) * m_cCurCharMapSize.y );
	m_bModified = true;
}

/**
 *
 */
void TermView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	uint32 nChar;

	if( nQualifiers & QUAL_DEADKEY ) {
		return;
	}

	if( ( nQualifiers & QUAL_SHIFT ) && pzRawString[0] == VK_INSERT )
	{
		Paste();
		return;
	}

	nChar = utf8_to_unicode( pzString );

	if( nQualifiers & QUAL_CTRL )
	{
		bool bExpandSel = false;

		if( m_bIsSelRangeValid && m_cSelectStart != m_cSelectEnd )
		{
			uint32 nButtons;

			GetMouse( NULL, &nButtons );
			if( nButtons & 0x01 )
			{
				bExpandSel = true;
			}

			if( pzRawString[0] == 'c' ) {
				Copy();
				return;
			}
		}
		switch ( pzRawString[0] )
		{
		case 0:
			break;
		case 'v':
			Paste();
			break;
		case VK_UP_ARROW:
			if( m_nScrollPos < m_nTotLineCnt - m_cCurCharMapSize.y )
			{
				m_nScrollPos++;
				g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );
				memset( m_pabModifiedLines, true, sizeof( m_pabModifiedLines[0] ) * m_cCurCharMapSize.y );
				if( bExpandSel )
				{
					RemoveSelection();
					m_cSelectEnd.y -= m_cCharSize.y;
					if( nQualifiers & QUAL_ALT )
					{
						m_bIsSelRangeRect = true;
					}
					else
					{
						m_bIsSelRangeRect = false;
					}
					HighlightSelection();
				}
				m_bModified = true;
			}
			break;
		case VK_DOWN_ARROW:
			if( m_nScrollPos > 0 )
			{
				m_nScrollPos--;
				g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );
				memset( m_pabModifiedLines, true, sizeof( m_pabModifiedLines[0] ) * m_cCurCharMapSize.y );
				if( bExpandSel )
				{
					RemoveSelection();
					m_cSelectEnd.y += m_cCharSize.y;
					if( nQualifiers & QUAL_ALT )
					{
						m_bIsSelRangeRect = true;
					}
					else
					{
						m_bIsSelRangeRect = false;
					}
					HighlightSelection();
				}
				m_bModified = true;
			}
			break;
		case VK_PAGE_UP:
			if( bExpandSel )
			{
				RemoveSelection();
			}
			if( m_nScrollPos + m_cCurCharMapSize.y < m_nTotLineCnt - m_cCurCharMapSize.y - 1 )
			{
				m_nScrollPos += m_cCurCharMapSize.y;
				if( bExpandSel )
				{
					m_cSelectEnd.y -= m_cCurCharMapSize.y * m_cCharSize.y;
				}
			}
			else
			{
				if( bExpandSel )
				{
					m_cSelectEnd.y += ( m_nScrollPos - ( m_nTotLineCnt - m_cCurCharMapSize.y ) ) * m_cCharSize.y;
				}
				m_nScrollPos = m_nTotLineCnt - m_cCurCharMapSize.y;
			}
			g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );
			memset( m_pabModifiedLines, true, sizeof( m_pabModifiedLines[0] ) * m_cCurCharMapSize.y );
			if( bExpandSel )
			{
				if( nQualifiers & QUAL_ALT )
				{
					m_bIsSelRangeRect = true;
				}
				else
				{
					m_bIsSelRangeRect = false;
				}
				HighlightSelection();
			}
			m_bModified = true;
			break;
		case VK_PAGE_DOWN:
			if( bExpandSel )
			{
				RemoveSelection();
			}
			if( m_nScrollPos > m_cCurCharMapSize.y )
			{
				if( bExpandSel )
				{
					m_cSelectEnd.y += m_cCurCharMapSize.y * m_cCharSize.y;
				}
				m_nScrollPos -= m_cCurCharMapSize.y;
			}
			else
			{
				if( bExpandSel )
				{
					m_cSelectEnd.y += m_nScrollPos * m_cCharSize.y;
				}
				m_nScrollPos = 0;
			}
			g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );
			memset( m_pabModifiedLines, true, sizeof( m_pabModifiedLines[0] ) * m_cCurCharMapSize.y );
			if( bExpandSel )
			{
				if( nQualifiers & QUAL_ALT )
				{
					m_bIsSelRangeRect = true;
				}
				else
				{
					m_bIsSelRangeRect = false;
				}
				HighlightSelection();
			}

			m_bModified = true;
			break;
		case VK_HOME:
			if( bExpandSel )
			{
				RemoveSelection();
				m_cSelectEnd.y += ( m_nScrollPos - ( m_nTotLineCnt - m_cCurCharMapSize.y ) ) * m_cCharSize.y;
				if( nQualifiers & QUAL_ALT )
				{
					m_bIsSelRangeRect = true;
				}
				else
				{
					m_bIsSelRangeRect = false;
				}
				HighlightSelection();
			}
			m_nScrollPos = m_nTotLineCnt - m_cCurCharMapSize.y;
			g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );
			memset( m_pabModifiedLines, true, sizeof( m_pabModifiedLines[0] ) * m_cCurCharMapSize.y );
			m_bModified = true;
			break;
		case VK_END:
			if( bExpandSel )
			{
				RemoveSelection();
				m_cSelectEnd.y += m_nScrollPos * m_cCharSize.y;
				if( nQualifiers & QUAL_ALT )
				{
					m_bIsSelRangeRect = true;
				}
				else
				{
					m_bIsSelRangeRect = false;
				}
				HighlightSelection();
			}

			m_nScrollPos = 0;
			g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );
			memset( m_pabModifiedLines, true, sizeof( m_pabModifiedLines[0] ) * m_cCurCharMapSize.y );
			m_bModified = true;
			break;
		case VK_INSERT:
			Copy();
			break;
		case VK_RIGHT_ARROW:
			break;
		case VK_LEFT_ARROW:
			break;
		default:
			write( g_nMasterPTY, pzString, utf8_char_length( pzString[0] ) );
			break;
		}
	}
	else
	{
		if( 0 != nChar && 0 != m_nScrollPos )
		{
			m_nScrollPos = 0;
			g_pcScrollBar->SetValue( m_nTotLineCnt - m_nScrollPos - m_cCurCharMapSize.y );
			memset( m_pabModifiedLines, true, sizeof( m_pabModifiedLines[0] ) * m_cCurCharMapSize.y );
			m_bModified = true;
		}

		switch ( nChar )
		{
		case 0:
			break;
		case VK_UP_ARROW:
			if( m_bAlternateCursorKeys )
			{
				write( g_nMasterPTY, "\x1bOA", 3 );
			}
			else
			{
				write( g_nMasterPTY, "\x1b[A", 3 );
			}
			break;
		case VK_DOWN_ARROW:
			if( m_bAlternateCursorKeys )
			{
				write( g_nMasterPTY, "\x1bOB", 3 );
			}
			else
			{
				write( g_nMasterPTY, "\x1b[B", 3 );
			}
			break;
		case VK_RIGHT_ARROW:
			if( m_bAlternateCursorKeys )
			{
				write( g_nMasterPTY, "\x1bOC", 3 );
			}
			else
			{
				write( g_nMasterPTY, "\x1b[C", 3 );
			}
			break;
		case VK_LEFT_ARROW:
			if( m_bAlternateCursorKeys )
			{
				write( g_nMasterPTY, "\x1bOD", 3 );
			}
			else
			{
				write( g_nMasterPTY, "\x1b[D", 3 );
			}
			break;
		case VK_END:
			write( g_nMasterPTY, "\x05", 1 );
			break;
		case VK_PAGE_UP:
			write( g_nMasterPTY, "\x1b[5~", 4 );
			break;
		case VK_PAGE_DOWN:
			write( g_nMasterPTY, "\x1b[6~", 4 );
			break;
		case VK_DELETE:
			write( g_nMasterPTY, "\x1b[3~", 4 );
			break;
		case VK_BACKSPACE:
			write( g_nMasterPTY, "\x08", 1 );
			break;
		case VK_ENTER:
			write( g_nMasterPTY, "\r", 1 );
			break;
		case VK_INSERT:
			write( g_nMasterPTY, "\x1b[2~", 4 );
			break;
		case VK_FUNCTION_KEY:
			{
				Looper *pcLooper = GetLooper();

				assert( pcLooper != NULL );
				Message *pcMsg = pcLooper->GetCurrentMessage();

				assert( pcMsg != NULL );
				int32 nKeyCode;

				if( pcMsg->FindInt32( "_raw_key", &nKeyCode ) != 0 )
				{
					return;
				}

				switch ( nKeyCode )
				{
				case 2:	// F1
					write( g_nMasterPTY, "\x1b[11~", 5 );
					break;
				case 3:	// F2
					write( g_nMasterPTY, "\x1b[12~", 5 );
					break;
				case 4:	// F3
					write( g_nMasterPTY, "\x1b[13~", 5 );
					break;
				case 5:	// F4
					write( g_nMasterPTY, "\x1b[14~", 5 );
					break;
				case 6:	// F5
					write( g_nMasterPTY, "\x1b[15~", 5 );
					break;
				case 7:	// F6
					write( g_nMasterPTY, "\x1b[17~", 5 );
					break;
				case 8:	// F7
					write( g_nMasterPTY, "\x1b[18~", 5 );
					break;
				case 9:	// F8
					write( g_nMasterPTY, "\x1b[19~", 5 );
					break;
				case 10:	// F9
					write( g_nMasterPTY, "\x1b[20~", 5 );
					break;
				case 11:	// F10
					write( g_nMasterPTY, "\x1b[21~", 5 );
					break;
				case 12:	// F11
					write( g_nMasterPTY, "\x1b[23~", 5 );
					break;
				case 13:	// F12
					write( g_nMasterPTY, "\x1b[24~", 5 );
					break;


				}
				break;
			}
		default:
			write( g_nMasterPTY, pzString, utf8_char_length( pzString[0] ) );
			break;
		}
	}
}



