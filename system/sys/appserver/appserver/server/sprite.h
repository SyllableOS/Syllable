#ifndef __F_SRVSPRITE_H__
#define __F_SRVSPRITE_H__

/*
 *  The AtheOS application server
 *  Copyright (C) 1999  Kurt Skauen
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

#include <vector>
#include <gui/region.h>
#include <util/locker.h>


using namespace os;


class SrvBitmap;

class SrvSprite
{
public:
    SrvSprite( const IRect& cFrame, const IPoint& cPos, const IPoint& cHotSpot,
	       SrvBitmap* pcTarget, SrvBitmap* pcImage );
    ~SrvSprite();

    IRect GetBounds() const { return( m_cBounds ); }
    void Draw();
    void Erase();

    void Draw( SrvBitmap* pcTarget, const IPoint& cPos );
    void Capture( SrvBitmap* pcTarget, const IPoint& cPos );
    void Erase( SrvBitmap* pcTarget, const IPoint& cPos );
  
    void MoveBy( const IPoint& cDelta );
    void MoveTo( const IPoint& cNewPos ) { MoveBy( cNewPos - m_cPosition ); }

    static void ColorSpaceChanged();
    static void Hide( const IRect& cFrame );
    static void Hide();
    static void Unhide();
  
private:
    static std::vector<SrvSprite*> s_cInstances;
    static atomic_t	   	   s_nHideCount;
    static Locker		   s_cLock;

    IPoint     m_cPosition;
    IPoint     m_cHotSpot;
    SrvBitmap* m_pcImage;
    SrvBitmap* m_pcTarget;
    SrvBitmap* m_pcBackground;
    IRect      m_cBounds;
    bool       m_bVisible;
};


#endif // __F_SRVSPRITE_H__
