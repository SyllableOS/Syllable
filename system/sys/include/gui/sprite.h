/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999  Kurt Skauen
 *  Copyright (C) 2005  The Syllable Team
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

#ifndef __F_GUI_SPRITE_H__
#define __F_GUI_SPRITE_H__

#include <gui/region.h>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent


class Bitmap;

class Sprite
{
public:
    Sprite( const Point& cPosition, Bitmap* pcBitmap );
    ~Sprite();

    void MoveBy( const Point& cDelta );
    void MoveTo( const Point& cNewPos );
  
private:
    Sprite& operator=( const Sprite& );
    Sprite( const Sprite& );

    uint32 m_nHandle;
    Point m_cPosition;
};

} // End of namespace

#endif // __F_GUI_SPRITE_H__

