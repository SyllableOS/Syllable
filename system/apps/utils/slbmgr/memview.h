/*
 *  AtheMgr - System Manager for AtheOS
 *  Copyright (C) 2001 John Hall
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

#ifndef __F_MEMVIEW_H__
#define __F_MEMVIEW_H__

using namespace os;

class MemMeter : public View
{
public:
    MemMeter( const Rect& cFrame, const char* pzTitle,
		uint32 nResizeMask, uint32 nFlags = WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE );
    ~MemMeter();

      // From View:
    virtual Point GetPreferredSize( bool bLargest ) const { return( (bLargest) ? Point( 100000, 100000 ) : Point( 5, 5 ) );}
    virtual void	Paint( const Rect& cUpdateRect );
    virtual void	MouseDown( const Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );

    void	AddValue( float vTotMem, float vCacheSize, float vDirtyCache );
private:
    //Menu* m_pcPopupMenu;
    float m_avTotUsedMem[ 1024 ];
    float m_avCacheSize[ 1024 ];
    float m_avDirtyCacheSize[ 1024 ];

};

#endif // __F_MEMVIEW_H__
