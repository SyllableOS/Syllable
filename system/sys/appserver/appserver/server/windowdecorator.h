/*
 *  The Syllable application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#ifndef __F_WINDOWDECORATOR_H__
#define __F_WINDOWDECORATOR_H__

#include <atheos/types.h>
#include <gui/region.h>

class Layer;

namespace os {
#if 0
} // Fool Emacs auto-indent
#endif
  enum { WNDDECORATOR_APIVERSION = 6 };
  
class WindowDecorator
{
public:
    enum hit_item {
			HIT_NONE,
			HIT_CLOSE, HIT_ZOOM, HIT_DEPTH, HIT_MINIMIZE, HIT_STICKY, HIT_DRAG,
			// Note!
			// Keep these items in sequence, especially those above this line.
			HIT_SIZE_LEFT, HIT_SIZE_TOP, HIT_SIZE_RIGHT, HIT_SIZE_BOTTOM,
			HIT_SIZE_LT, HIT_SIZE_RT, HIT_SIZE_LB, HIT_SIZE_RB,
			HIT_MAX_ITEMS };
  
    WindowDecorator( Layer* pcLayer, uint32 nWndFlags );
    virtual ~WindowDecorator();
  
    Layer* GetLayer() const;

    virtual hit_item HitTest( const Point& cPosition ) = 0;
    virtual void FrameSized( const Rect& cNewFrame ) = 0;
    virtual Rect GetBorderSize() = 0;
    virtual Point GetMinimumSize() = 0;
    virtual void SetTitle( const char* pzTitle ) = 0;
    virtual void SetFlags( uint32 nFlags ) = 0;
    virtual void FontChanged() = 0;
    virtual void SetWindowFlags( uint32 nFlags ) = 0;
    virtual void SetFocusState( bool bHasFocus ) = 0;
    virtual void SetButtonState( uint32 nButton, bool bPushed ) = 0;
    virtual void Render( const Rect& cUpdateRect ) = 0;

private:
    Layer* m_pcLayer;
};

typedef int op_get_decorator_version();
typedef WindowDecorator* op_create_decorator( Layer* pcView, uint32 nFlags );
typedef int op_unload_decorator();

}

#endif // __F_WINDOWDECORATOR_H__

