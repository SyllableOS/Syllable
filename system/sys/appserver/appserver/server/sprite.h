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

class SrvBitmap;

class SrvSprite
{
public:
    SrvSprite( const os::IRect& cFrame, const os::IPoint& cPos, const os::IPoint& cHotSpot,
	       SrvBitmap* pcTarget, SrvBitmap* pcImage, bool bLastPos = false );
    ~SrvSprite();

    os::IRect GetBounds() const { return( m_cBounds ); }
    void Draw();
    void Erase();

    void Draw( SrvBitmap* pcTarget, const os::IPoint& cPos );
    void Capture( SrvBitmap* pcTarget, const os::IPoint& cPos );
    void Erase( SrvBitmap* pcTarget, const os::IPoint& cPos );
  
    void MoveBy( const os::IPoint& cDelta );
    void MoveTo( const os::IPoint& cNewPos ) { MoveBy( cNewPos - m_cPosition ); }

    static void ColorSpaceChanged();
    static void Hide( const os::IRect& cFrame );
    static void Hide();
    static void HideForCopy( const os::IRect& cFrame );
    static void Unhide();
  	static bool DoIntersect( const os::IRect& cRect );
  	static bool CoveredByRect( const os::IRect& cRect );
  
private:
    static std::vector<SrvSprite*> s_cInstances;
    static atomic_t	   	   s_nHideCount;
    static os::Locker	   s_cLock;

    os::IPoint m_cPosition;
    os::IPoint m_cHotSpot;
    SrvBitmap* m_pcImage;
    SrvBitmap* m_pcTarget;
    SrvBitmap* m_pcBackground;
    os::IRect  m_cBounds;
    bool       m_bVisible;
};


#endif // __F_SRVSPRITE_H__
