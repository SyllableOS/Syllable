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

#ifndef __F_CPUVIEW_H__
#define __F_CPUVIEW_H__

#include <atheos/kernel.h>
#include <gui/view.h>

namespace os {
    class TableView;
    class StringView;
}


class	CpuMeter : public os::View
{
public:
    CpuMeter( const os::Rect& cFrame, const char* pzTitle, uint32 nResizeMask, uint32 nFlags );
    ~CpuMeter();

      // From View:
    virtual os::Point GetPreferredSize( bool bLargest ) const {
	return( (bLargest) ? os::Point( 100000, 100000 ) : os::Point( 5, 5 ) );
    }
  
    virtual void Paint( const os::Rect& cUpdateRect );

    void AddValue( float vValue );

private:
    float m_avValues[ 1024 ];
};


#endif // __F_CPUVIEW_H__
