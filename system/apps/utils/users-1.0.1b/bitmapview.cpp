/*
 *  Users and Passwords Manager for AtheOS
 *  Copyright (C) 2002 William Rose
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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
#include "bitmapview.h"

namespace ui {
  BitmapView::BitmapView( const os::Rect& cFrame,
                          const std::string& cName,
                          os::Bitmap *pcBitmap,
                          uint32 nResizeMask,
                          uint32 nFlags )
    : View( cFrame, cName, nResizeMask, nFlags ), m_pcBitmap( pcBitmap ) {
  }

  BitmapView::~BitmapView( ) {
    if( m_pcBitmap )
      delete m_pcBitmap;
  }

  void BitmapView::SetBitmap( os::Bitmap *pcBitmap ) {
    if( m_pcBitmap )
      delete m_pcBitmap;

    m_pcBitmap = pcBitmap;
  }
  
  void BitmapView::AttachedToWindow() {
    os::View *pcParent = GetParent();

    if( pcParent != NULL ) {
      SetBgColor( pcParent->GetBgColor() );
    }
  }

  void BitmapView::Paint( const os::Rect& cDamaged ) {
    SetDrawingMode( os::DM_COPY );
    FillRect( cDamaged, get_default_color( os::COL_NORMAL ) );

    // Set blending
    if( m_pcBitmap == NULL )
      return;

    os::Rect cBounds = m_pcBitmap->GetBounds();
    
    if( cBounds.DoIntersect( cDamaged ) ) {
      os::Rect cIntersect = cBounds & cDamaged;
      
      SetDrawingMode( os::DM_BLEND );
      
      DrawBitmap( m_pcBitmap, cIntersect, cIntersect );
    }
  }

  os::Point BitmapView::GetPreferredSize( bool bLargest ) const {
    if( m_pcBitmap == NULL )
      return os::Point( 0, 0 );

    return os::Point( m_pcBitmap->GetBounds().Width(),
                      m_pcBitmap->GetBounds().Height() );
  }

  os::Point BitmapView::GetContentSize( ) const {
    if( m_pcBitmap == NULL )
      return os::Point( 0, 0 );

    return os::Point( m_pcBitmap->GetBounds().Width(),
                      m_pcBitmap->GetBounds().Height() );
  }
};
