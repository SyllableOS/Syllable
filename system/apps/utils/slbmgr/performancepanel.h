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

#ifndef __F_PERFORMANCEPANEL_H__
#define __F_PERFORMANCEPANEL_H__

#include <atheos/kernel.h>
#include <gui/view.h>

#include <gui/layoutview.h>

#include "memview.h"
#include "cpuview.h"


using namespace os;

class PerformancePanel : public os::LayoutView
{
public:
  
    PerformancePanel( const os::Rect& cFrame );
    ~PerformancePanel();

    virtual void Paint( const os::Rect& cUpdateRect );
    virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void FrameSized( const os::Point& cDelta );
    virtual void AllAttached();
    virtual void HandleMessage( Message* pcMessage );

    void UpdatePerformanceList();
   
private:
   
    StringView*	m_pcMemUsageStr1;
    StringView* m_pcMemUsageStr2;
    StringView* m_pcMemUsageStr3;
    StringView* m_pcMemTitle;
    StringView* m_pcCpuTitle;
   
    StringView* m_pcMemTitleStr1;
    StringView* m_pcMemTitleStr2;
    StringView* m_pcMemTitleStr3;

    MemMeter*	m_pcMemUsage;
    
    StringView* m_pcCpuUsageStr[MAX_CPU_COUNT];
    CpuMeter* 	m_pcCpuUsage[MAX_CPU_COUNT];

    int		m_nCPUCount;
    int		m_nW8378xDevice;

};

#endif // __F_PERFORMANCEPANEL_H__
