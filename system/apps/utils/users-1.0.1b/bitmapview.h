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
#ifndef WR_UI_BITMAPVIEW_H
#define WR_UI_BITMAPVIEW_H

#include <gui/bitmap.h>
#include <gui/view.h>

namespace ui {
  class BitmapView : public os::View {
    public:
      BitmapView( const os::Rect& cFrame,
                  const std::string& cName,
                  os::Bitmap *pcBitmap = NULL,
                  uint32 nResizeMask = os::CF_FOLLOW_LEFT |
                                       os::CF_FOLLOW_TOP,
                  uint32 nFlags = os::WID_WILL_DRAW |
                                  os::WID_CLEAR_BACKGROUND );

      virtual ~BitmapView();

      void SetBitmap( os::Bitmap *pcBitmap );

      virtual void AttachedToWindow();

      virtual void Paint( const os::Rect& cDamaged );

      virtual os::Point GetPreferredSize( bool bLargest = true ) const;
      virtual os::Point GetContentSize(  ) const;

    private:
      os::Bitmap *m_pcBitmap;
  };
};

#endif
