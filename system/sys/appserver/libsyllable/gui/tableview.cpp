/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 The Syllable Team
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

#include <assert.h>
#include <gui/window.h>
#include <gui/tableview.h>
#include <gui/font.h>
#include <util/message.h>

using namespace os;

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

TableCell::TableCell()
{
    m_eHorAlign	= ALIGN_LEFT;
    m_eVerAlign	= ALIGN_CENTER;
    m_vHorWheight	= 1.0f;
    m_vVerWheight	= 1.0f;
    m_pcView	= NULL;
    m_nLeftBorder = m_nRightBorder = m_nTopBorder = m_nBottomBorder = 3;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

TableCell::~TableCell()
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableCell::SetView( View* pcView )
{
    m_pcView = pcView;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

View* TableCell::GetView( void ) const
{
    return( m_pcView );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableCell::SetWheights( float vHor, float vVer )
{
    m_vHorWheight	= vHor;
    m_vVerWheight	= vVer;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point TableCell::GetPreferredSize( bool bLargest ) const
{
    if ( NULL != m_pcView ) {
	Point	cSize = m_pcView->GetPreferredSize( bLargest );
	return( cSize + Point( m_nLeftBorder + m_nRightBorder, m_nTopBorder + m_nBottomBorder ) );
    } else {
	if ( bLargest ) {
	    return( Point( 10000, 10000 ) );
	} else {
	    return( Point( 0, 0 ) );
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableCell::SetAlignment( alignment eHor, alignment eVer )
{
    m_eHorAlign = eHor;
    m_eVerAlign = eVer;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableCell::SetWidth( int nWidth )
{
    m_cSize.x = nWidth;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableCell::SetHeight( int nHeight )
{
    m_cSize.y = nHeight;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableCell::SetSize( Point cSize )
{
    m_cSize = cSize;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableCell::SetPos( Point cPos )
{
    m_cPos = cPos;
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableCell::Layout( void )
{
    if ( NULL != m_pcView )
    {
	Point cSize = m_pcView->GetPreferredSize( true );

	Rect cBounds =
	    Rect( 0, 0, cSize.x - 1, cSize.y - 1 ) &
	    Rect( 0, 0, m_cSize.x - m_nLeftBorder - m_nRightBorder - 1, m_cSize.y - m_nTopBorder - m_nBottomBorder - 1 );
    
	Point cPos = m_cPos; // + Point( m_nLeftBorder, m_nTopBorder );


	switch( m_eHorAlign )
	{
	    case ALIGN_LEFT:
		cPos.x += m_nLeftBorder;
		break;
	    case ALIGN_CENTER:
		cPos.x += m_cSize.x / 2 - (cBounds.Width()+1.0f) / 2;
		break;
	    case ALIGN_RIGHT:
		cPos.x += m_cSize.x - cBounds.Width() + 1.0f - m_nRightBorder;
		break;
	    default:
		dbprintf( "Error: TableCell::Layout() invalid horizontal alignment %d\n", m_eHorAlign);
		break;
	}

	switch( m_eVerAlign )
	{
	    case ALIGN_TOP:
		cPos.y += m_nTopBorder;
		break;
	    case ALIGN_CENTER:
		cPos.y += m_cSize.y / 2 - (cBounds.Height()+1.0f) / 2;
		break;
	    case ALIGN_BOTTOM:
		cPos.y += m_cSize.y - cBounds.Height() + 1.0f - m_nBottomBorder;
		break;
	    default:
		dbprintf( "Error: TableCell::Layout() invalid vertical alignment %d\n", m_eHorAlign);
		break;
	}
	m_pcView->SetFrame( (cBounds + cPos).Floor() );
    }
}




//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

TableView::TableView( const Rect& cFrame, const String& cName, const String& cTitle,
		      int nNumCols, int nNumRows, uint32 nResizeMask )
    : View( cFrame, cName, nResizeMask, WID_WILL_DRAW )
{
    if ( !cTitle.empty() && !( cTitle == "" ) )
    {
	m_cTitle = cTitle;
	m_nLeftBorder = m_nRightBorder = m_nBottomBorder = 4;
	m_nTopBorder	=	14;	// Will be updated when added to window;
    }
    else
    {
	m_nLeftBorder = m_nRightBorder = m_nTopBorder = m_nBottomBorder = 0;
	m_cTitle.str().clear();
    }
    
    m_pacCells	  = new TableCell[ nNumCols * nNumRows ];
    m_panMinWidths  = new float[ nNumCols ];
    m_panMaxWidths  = new float[ nNumCols ];
    m_panMinHeights = new float[ nNumRows ];
    m_panMaxHeights = new float[ nNumRows ];

    m_pavColWheights = new float[ nNumCols ];
    m_pavRowWheights = new float[ nNumRows ];

    m_nNumCols	= nNumCols;
    m_nNumRows	= nNumRows;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

TableView::~TableView()
{
    delete [] m_pacCells;
    delete [] m_panMinWidths;
    delete [] m_panMaxWidths;
    delete [] m_panMinHeights;
    delete [] m_panMaxHeights;

    delete [] m_pavColWheights;
    delete [] m_pavRowWheights;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::AttachedToWindow( void )
{
    Window* pcWindow = GetWindow();

    assert( pcWindow != NULL );

    SetBgColor( get_default_color( COL_NORMAL ) );

    if ( m_cTitle.empty() ) {
	    m_nTopBorder = m_nBottomBorder;
	} else {
	    font_height sHeight;
	    GetFontHeight( &sHeight );
	    m_nTopBorder = sHeight.ascender + sHeight.descender + 2.0f;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::Layout()
{
    Layout( GetBounds() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::AllAttached( void )
{
    Layout( GetBounds() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::FrameSized( const Point& cDelta )
{
    Layout( GetBounds() );
    Invalidate( GetBounds() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::SetColAlignment( int nCol, alignment eAlign )
{
    int	y;

    for ( y = 0 ; y < m_nNumRows ; ++y )
    {
	TableCell*	pcCell = GetCell( nCol, y );
	if ( NULL != pcCell ) {
	    pcCell->SetAlignment( eAlign, ALIGN_CENTER );
	}
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::SetCellAlignment( int x, int y, alignment eHor, alignment eVer )
{
    TableCell*	pcCell = GetCell( x, y );

    if ( NULL != pcCell ) {
	pcCell->SetAlignment( eHor, eVer );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::SetCellBorders( int x, int y, int nLeft, int nTop, int nRight, int nBottom )
{
    TableCell*	pcCell = GetCell( x, y );

    if ( NULL != pcCell )
    {
	pcCell->m_nLeftBorder 	= nLeft;
	pcCell->m_nTopBorder 	= nTop;
	pcCell->m_nRightBorder 	= nRight;
	pcCell->m_nBottomBorder = nBottom;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::SetCellBorders( int x, int nLeft, int nTop, int nRight, int nBottom )
{
    int y;

    for ( y = 0 ; y < m_nNumRows ; ++y ) {
	SetCellBorders( x, y, nLeft, nTop, nRight, nBottom );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::SetCellBorders( int nLeft, int nTop, int nRight, int nBottom )
{
    int x;

    for ( x = 0 ; x < m_nNumCols ; ++x ) {
	SetCellBorders( x, nLeft, nTop, nRight, nBottom );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

TableCell* TableView::GetCell( int x, int y ) const
{
    if ( x < m_nNumCols && y < m_nNumRows ) {
	return( &m_pacCells[ x + y * m_nNumCols ] );
    } else {
	return( NULL );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

View* TableView::SetChild( View* pcView, int x, int y, float vHorWheight, float vVerWheight )
{
    View* pcPrevView = NULL;

    TableCell*	pcCell = GetCell( x, y );

    if ( NULL != pcCell ) {
	pcPrevView = pcCell->GetView();

	if ( NULL != pcPrevView ) {
	    RemoveChild( pcPrevView );
	}

	pcCell->SetView( pcView );
	pcCell->SetWheights( vHorWheight, vVerWheight );
	if ( NULL != pcView ) {
	    AddChild( pcView );
	}
    }
    return( pcPrevView );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::CalcSizes( void ) const
{
    int	x;
    int	y;

    m_nMinWidth	 = 0;
    m_nMaxWidth	 = 0;
    m_nMinHeight = 0;
    m_nMaxHeight = 0;

    for ( x = 0 ; x < m_nNumCols ; ++x ) {
	m_panMaxWidths[x] 	= 0;
	m_panMinWidths[x] 	= 0;
	m_pavColWheights[x] = 0.0f;
    }

    for ( y = 0 ; y < m_nNumRows ; ++y ) {
	m_panMinHeights[y] 	= 0;
	m_panMaxHeights[y] 	= 0;
	m_pavRowWheights[y] = 0.0f;
    }

    for ( x = 0 ; x < m_nNumCols ; ++x )
    {
	for ( y = 0 ; y < m_nNumRows ; ++y )
	{
	    TableCell*	pcCell = GetCell( x, y );

	    Point	cMinSize = pcCell->GetPreferredSize( false );
	    Point	cMaxSize = pcCell->GetPreferredSize( true );

	    m_pavColWheights[ x ] += pcCell->m_vVerWheight;
	    m_pavRowWheights[ y ] += pcCell->m_vHorWheight;


	    if ( cMinSize.x > m_panMinWidths[x] ) {
		m_panMinWidths[x] = cMinSize.x;
	    }
	    if ( cMaxSize.x > m_panMaxWidths[x] ) {
		m_panMaxWidths[x] = cMaxSize.x;
	    }
	    if ( cMinSize.y > m_panMinHeights[y] ) {
		m_panMinHeights[y] = cMinSize.y;
	    }
	    if ( cMaxSize.y > m_panMaxHeights[y] ) {
		m_panMaxHeights[y] = cMaxSize.y;
	    }
	}
    }
    for ( x = 0 ; x < m_nNumCols ; ++x ) {
	m_nMinWidth += m_panMinWidths[x];
	m_nMaxWidth += m_panMaxWidths[x];
    }

    for ( y = 0 ; y < m_nNumRows ; ++y ) {
	m_nMinHeight += m_panMinHeights[y];
	m_nMaxHeight += m_panMaxHeights[y];
    }

}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::Layout( Rect cBounds )
{
    int	x;
    int	y;

    CalcSizes();

    cBounds.left   += m_nLeftBorder;
    cBounds.top	   += m_nTopBorder;
    cBounds.right  -= m_nRightBorder;
    cBounds.bottom -= m_nBottomBorder;

/*
  float	vHorAspect = (float) cBounds.Width() / (float) nWidth;
  float	vVerAspect = (float) cBounds.Height() / (float) nHeight;
  */

    Point	cPos( 0, 0 );

    float	nDeltaW	= cBounds.Width() + 1.0f - m_nMinWidth;
    float	nDeltaH	= cBounds.Height() + 1.0f - m_nMinHeight;

    float nXSpacing = (cBounds.Width()+1.0f > m_nMaxWidth) ? ((cBounds.Width() + 1.0f - m_nMaxWidth) / m_nNumCols) : 0;
    float nYSpacing = (cBounds.Height()+1.0f > m_nMaxHeight) ? ((cBounds.Height() + 1.0f - m_nMaxHeight)	/ m_nNumRows) : 0;

    for ( x = 0 ; x < m_nNumCols ; ++x )
    {
	float	nDeltaX = 0.0f;
	float	nDeltaY;

	cPos.y = 0;

	for ( y = 0 ; y < m_nNumRows ; ++y )
	{
	    TableCell* pcCell = GetCell( x, y );

	    if ( (m_nMaxWidth - m_nMinWidth) != 0 ) {
		nDeltaX = nDeltaW * (m_panMaxWidths[x] - m_panMinWidths[x]) / (m_nMaxWidth - m_nMinWidth);
	    } else {
		nDeltaX = 0;
	    }
	    if ( (m_nMaxHeight - m_nMinHeight) != 0 ) {
		nDeltaY = nDeltaH * (m_panMaxHeights[y] - m_panMinHeights[y]) / (m_nMaxHeight - m_nMinHeight);
	    } else {
		nDeltaY	= 0;
	    }

	    pcCell->SetSize( Point( m_panMinWidths[x] + nDeltaX, m_panMinHeights[y] + nDeltaY ) );
	    pcCell->SetPos( cPos + Point( m_nLeftBorder, m_nTopBorder ) );
	    pcCell->Layout();

	    cPos.y += m_panMinHeights[y] + nDeltaY + nYSpacing;
	}
	cPos.x += m_panMinWidths[x] + nDeltaX + nXSpacing;
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Point TableView::GetPreferredSize( bool bLargest ) const
{
    CalcSizes();

    if ( bLargest ) {
	return( Point( 10000, 10000 ) );
//		return( Point( m_nMaxWidth, m_nMaxHeight ) );
    } else {
	return( Point( m_nMinWidth + m_nLeftBorder + m_nRightBorder, m_nMinHeight + m_nTopBorder + m_nBottomBorder ) );
    }
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void TableView::Paint( const Rect& cUpdateRect )
{
	Window*	pcWindow = GetWindow();

	if ( NULL != pcWindow )
	{
		Rect	cBounds	=	GetBounds();

		SetFgColor( get_default_color( COL_NORMAL ) );
		FillRect( GetBounds() );

	    SetFgColor( 0, 0, 0, 0 );

	    Font*	pcFont	=	GetFont();

	    if ( NULL != pcFont )
	    {
			float	x = 20.0f;
			float	vStrWidth = 0.0f;

			if (  !m_cTitle.empty() ) {
				font_height sHeight;

				pcFont->GetHeight( &sHeight );
				float vStrHeight = sHeight.ascender + sHeight.descender;
				
				vStrWidth = pcFont->GetStringWidth( m_cTitle );
				
				cBounds.top = vStrHeight / 2.0f;
				DrawString( Point( x, sHeight.ascender ), m_cTitle );
			}

			// Left
			DrawLine( Point( cBounds.left, cBounds.bottom ), Point( cBounds.left, cBounds.top ) );

			// Top
			if ( m_cTitle.empty() )
			{
				DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
			}
			else
			{
				DrawLine( Point( cBounds.left, cBounds.top ), Point( x - 3, cBounds.top ) );
				DrawLine( Point( x + vStrWidth + 3.0f, cBounds.top ), Point( cBounds.right, cBounds.top ) );
			}
			// Right
			DrawLine( Point( cBounds.right - 1, cBounds.top ), Point( cBounds.right - 1, cBounds.bottom ) );
			// Bottom
			DrawLine( Point( cBounds.left + 2, cBounds.bottom - 1 ), Point( cBounds.right - 1, cBounds.bottom - 1 ) );


			SetFgColor( 255, 255, 255 );
			// Left
			DrawLine( Point( cBounds.left + 1, cBounds.bottom ), Point( cBounds.left + 1, cBounds.top + 1 ) );
			// Top
			if ( m_cTitle.empty() )
			{
				DrawLine( Point( 1, cBounds.top + 1 ), Point( cBounds.right, cBounds.top + 1 ) );
			}
			else
			{
				DrawLine( Point( 1, cBounds.top + 1 ), Point( x - 3, cBounds.top + 1 ) );
				DrawLine( Point( x + vStrWidth + 3.0f, cBounds.top + 1 ), Point( cBounds.right, cBounds.top + 1 ) );
			}
			// Right
			DrawLine( Point( cBounds.right, cBounds.top + 1 ), Point( cBounds.right, cBounds.bottom ) );
			// Bottom
			DrawLine( Point( cBounds.left + 1, cBounds.bottom ), Point( cBounds.right, cBounds.bottom ) );
		}
	}
}
