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

#ifndef __F_GRAPHVIEW_H__
#define __F_GRAPHVIEW_H__

#include <atheos/kernel.h>
#include <gui/view.h>
#include <gui/layoutview.h>

namespace os {
    class TableView;
    class StringView;
}
class MultiMeter;


class GraphView : public os::View
{
public:
    GraphView( const os::Rect& cRect, int nCPUCount );
    ~GraphView();

    virtual void      AttachedToWindow();
    virtual void      DetachedFromWindow();
    virtual void      TimerTick( int nID );
    
    
private:
    os::LayoutView*	m_pcLayoutView;
    os::VLayoutNode* m_pcVLayout;
    MultiMeter*		m_apcMeters[MAX_CPU_COUNT];
    os::StringView*	m_apcTempViews[MAX_CPU_COUNT];
    int			m_nCPUCount;
    int			m_nTimer;
    int	       		m_nW8378xDevice;
};




class	MultiMeter : public os::View
{
public:
    MultiMeter( const os::Rect& cFrame, const char* pzTitle, uint32 nResizeMask, uint32 nFlags );
    ~MultiMeter();


      // From View:
    virtual os::Point GetPreferredSize( bool bLargest ) const {
	return( (bLargest) ? os::Point( 100000, 100000 ) : os::Point( 5, 5 ) );
    }
  
    virtual void Paint( const os::Rect& cUpdateRect );

    void AddValue( float vValue );

private:
    float m_avValues[ 1024 ];

};




#endif // __F_GRAPHVIEW_H__
