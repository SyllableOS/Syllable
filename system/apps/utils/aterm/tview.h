#ifndef _TVIEW_HPP_
#define _TVIEW_HPP_

/*
 *  aterm - x-term like terminal emulator for AtheOS
 *  Copyright (C) 1999  Kurt Skauen
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

#include <gui/view.h>
#include <gui/font.h>
#include <gui/menu.h>

#include "settings.h"


#define VERSION "1.0.0c"

using namespace os;

extern ScrollBar *g_pcScrollBar;

extern int g_nMasterPTY;
extern int g_nSlavePTY;
extern int g_nDebugLevel;

#define	ATTR_FG_MASK	0x000f
#define ATTR_BG_MASK	0x00f0
#define	ATTR_BOLD	0x0100
#define	ATTR_DIM	0x0200
#define	ATTR_UNDERLINE	0x0400
#define	ATTR_BLINK	0x0800
#define	ATTR_INVERSE	0x1000
#define ATTR_INVISIBLE	0x2000
#define ATTR_SELECTED	0x4000
#define ATTR_HIGH_CHAR	0x8000


extern int g_nDefaultAttribs;

/**
 * This class stores one line of text
 */
class TextLine
{
      public:
	enum
	{ MAX = 10000 };

      public:
	  TextLine( int32 cChar = ' ', int nAttr = g_nDefaultAttribs );
	 ~TextLine();

	void Clear( int nStart = 0, int nEnd = MAX, int32 cChar = ' ', int nAttr = g_nDefaultAttribs );

	int m_nCurLength;
	int m_nMaxLength;

	int32 *m_pCharMap;
	int16 *m_pAttribMap;
};

/**
 * Message codes
 */
enum
{
};

/**
 * This class implements the whole terminal control
 */
class TermView:public View
{
      public:
	TermView( const Rect & cFrame, TermSettings * pcSettings, const char *pzTitle, uint32 nResizeMask, uint32 nFlags );
	 ~TermView();

	// From View:
	virtual void AttachedToWindow( void );
	virtual void Paint( const Rect & cUpdateRect );
	virtual void KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers );
	virtual void MouseDown( const Point & cPosition, uint32 nButtons );
	virtual void MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData );
	virtual void MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData );
	void FrameSized( const Point & cDelta );
	void HandleMessage( Message * pcMsg );

	// From TermView:

	int Write( const char *pBuffer, int nSize );

	void ScrollBack( int npos );

	void RefreshDisplay( bool bAll );
	Point GetGlyphSize() const
	{
		return ( m_cCharSize );
	}
	IPoint PixToChar( const Point & cPos )
	{
		return ( IPoint( ( int )( cPos.x / m_cCharSize.x ), ( int )( ( ( cPos.y >= 0 ) ? cPos.y : ( cPos.y - m_cCharSize.y ) ) / m_cCharSize.y ) ) );
	}
	Point CharToPix( const IPoint & cPos )
	{
		return ( Point( cPos.x * m_cCharSize.x, cPos.y * m_cCharSize.y ) );
	}
      private:
	Color32_s GetAttrFgColor( int nAttrib );
	Color32_s GetAttrBgColor( int nAttrib );
	uint GetStartBoundary( TextLine * pcLineBuf, uint pos );
	uint GetEndBoundary( TextLine * pcLineBuf, uint pos );
	void SortSelection( IPoint * pcPnt1, IPoint * pcPnt2 );
	void HighlightSelection();
	void RemoveSelection();
	void Copy();
	void Paste();

	void ExpandCharMap( IPoint cNewSize );
	void Reset()
	{
	}

	void Scroll( int nDeltaY, int y1, int y2 );

	void InvalideEsc();
	void PrintEscStr();
	void MoveCsr( int x, int y, bool bScroll );
	void SetCsr( int x, int y, bool bScroll );
	void InsertLines( int nCount );
	void DeleteLines( int nCount );

	void DeleteChars( int nCount );
	void InsertChars( int nCount );

	void EraseFromTopToCursor();
	void EraseToBottom();
	void EraseScreen();

	void PutChar( int32 nChar );

	void EraseToEndOfLine();
	void EraseToCursor();
	void EraseLine();

	void SetScrollWindow( int y1, int y2 );

	void ParseBraceEsc();
	void ParseParanEsc();
	void AddEscCode( char nChar );

	void RenderCursor( int nAttrib );
	void ClearLinePart( TextLine * pcLine, int x1, int x2, int y );
	void RenderLinePart( TextLine * pcLine, int x1, int x2, int y, int nAscender, bool bOnlyModified );
	void RenderLine( int y, int nAscender, bool bOnlyModified );


	TextLine *GetLineBuf( int y, bool bWidthScroll = false ) const;
	TextLine *GetPrevLineBuf( int y ) const;
	void InvalidateLine( int y );

	TermSettings *m_pcSettings;

	IPoint m_cSavedCsrPos;
	int m_nSavedAttribs;

	int m_nKeyPadMode;
	int m_nCharSet;

	int m_nCurrentBuffer;

	IPoint m_cCsrPos;
	IPoint m_cLastCsrPos;

	int m_nScrollTop;
	int m_nScrollBottom;

	IPoint m_cMaxCharMapSize;
	IPoint m_cCurCharMapSize;

	Point m_cCharSize;
	font_height m_sFontHeight;
	char m_zEscString[512];
	int m_nEscStringSize;

	bool *m_pabModifiedLines;
	bool m_bModified;

	TextLine **m_pBuffer;
	TextLine **m_pPrevBuffer;

	Menu *m_pcMenu;

	int m_nMaxLineCount;
	int m_nTotLineCnt;

	Point m_cSelectStart;
	Point m_cSelectEnd;
	bool m_bIsSelecting;
	bool m_bIsSelectingWords;
	bool m_bIsSelRangeValid;
	bool m_bIsSelRangeRect;	/* we are defining a rectangular selection */
	bigtime_t m_nLastClickTime;	/* to determine if user is double-clicking... */

	bool m_bAlternateCursorKeys;
	int m_nScrollPos;

	int16 m_nCurAttrib;
	bool m_bInsertMode;
};


#endif // _TVIEW_HPP_

