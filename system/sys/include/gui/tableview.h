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

#ifndef __F_GUI_TABLEVIEW_H__
#define __F_GUI_TABLEVIEW_H__

#include <gui/view.h>

namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

class	TableCell
{
public:
    TableCell();
    ~TableCell();

    void	SetView( View* pcView );
    View*	GetView( void ) const;

    void	SetWheights( float vHorizontal, float vVertical );
    Point	GetPreferredSize( bool bLargest ) const;
    void	SetAlignment( alignment eHorizontal, alignment hVertical );

    void	SetWidth( int nWidth );
    void	SetHeight( int nHeight );

    void	SetSize( Point cSize );
    void	SetPos( Point cPos );
    void	Layout( void );

private:
    friend class TableView;

    View*	m_pcView;

    Point	m_cSize;
    Point	m_cPos;

    float	m_vHorWheight;
    float	m_vVerWheight;

    alignment	m_eHorAlign;
    alignment	m_eVerAlign;

    int		m_nLeftBorder;
    int		m_nRightBorder;
    int		m_nTopBorder;
    int		m_nBottomBorder;
};

class	TableView : public View
{
public:
  TableView( const Rect& cFrame, const String& cName, const String& cTitle,
	     int nWidth, int nHeight, uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
  ~TableView();

    // From View:
  void		AttachedToWindow( void );
  void		AllAttached( void );
  void		FrameSized( const Point& cDelta );
  void		Paint( const Rect& cUpdateRect );
  Point		GetPreferredSize( bool bLargest ) const;


    // From TableView:
  void		SetColAlignment( int nCol, alignment eAlign );
  void		SetCellAlignment( int x, int y, alignment eHor, alignment eVer = ALIGN_CENTER );

  void		SetCellBorders( int x, int y, int nLeft, int nTop, int nRight, int nBottom );
  void		SetCellBorders( int x, int nLeft, int nTop, int nRight, int nBottom );
  void		SetCellBorders( int nLeft, int nTop, int nRight, int nBottom );

  View*		SetChild( View* pcView, int nColumn, int nRow, float vHorWheight = 1.0f, float vVerWheight = 1.0f );
  TableCell*	GetCell( int x, int y ) const;
  void		Layout();

private:
  void		CalcSizes( void ) const;
  void		Layout( Rect cBounds );

  TableCell*		m_pacCells;

  mutable float*	m_panMinWidths;
  mutable float*	m_panMaxWidths;

  mutable float*	m_panMinHeights;
  mutable float*	m_panMaxHeights;

  mutable float*	m_pavColWheights;
  mutable float*	m_pavRowWheights;

  mutable float		m_nMinWidth;
  mutable float		m_nMaxWidth;
  mutable float		m_nMinHeight;
  mutable float		m_nMaxHeight;

  float			m_nLeftBorder;
  float			m_nRightBorder;
  float			m_nTopBorder;
  float			m_nBottomBorder;
	String		m_cTitle;

  int			m_nNumRows;
  int			m_nNumCols;
};

}

#endif // __F_GUI_TABLEVIEW_H__

