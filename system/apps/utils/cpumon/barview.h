/*  CPUMon - Small AtheOS utility for displaying the CPU load.
 *  Copyright (C) 2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __F_BARVIEW_H__
#define __F_BARVIEW_H__

#include <atheos/kernel.h>
#include <gui/view.h>
#include <gui/font.h>


class BarView : public os::View
{
public:
    BarView( const os::Rect& cFrame, int nCPUCount );

    void    SetLoads( float* pavLoads );

    virtual void      AttachedToWindow();
    virtual void      DetachedFromWindow();
    virtual os::Point GetPreferredSize( bool bLargest ) const;
    virtual void      Paint( const os::Rect& cUpdateRect );
    virtual void      TimerTick( int nID );
    
private:
    os::Rect GetBarRect( int nCol, int nBar );
    void DrawBar( int nIndex );
    
    int	             m_nCPUCount;
    int              m_anNumLighted[MAX_CPU_COUNT];
    float            m_vNumWidth;
    os::font_height  m_sFontHeight;
};






#endif // __F_BARVIEW_H__
