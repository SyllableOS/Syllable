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

#ifndef __F_PROCESSPANEL_H_
#define __F_PROCESSPANEL_H_

#include <gui/layoutview.h>

/*#include <vector>
#include <map>*/

using namespace os;

namespace os {
  class Button;
  class ListView;
  class StringView;
}

class ProcessPanel : public os::LayoutView
{
public:
  
    ProcessPanel( const os::Rect& cFrame );
    virtual void Paint( const os::Rect& cUpdateRect );
    virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void FrameSized( const os::Point& cDelta );
    virtual void AllAttached();
    virtual void HandleMessage( Message* pcMessage );

    void UpdateProcessList();
   
    int  ThreadCount();
    void SetThreadCount( int nVal );
    int  GetThreadCount();
   
 private:

    int         nThreads;
   
    Button*	m_pcEndProcBut;  
    ListView*	m_pcProcessList;
};

#endif // __F_PROCESSPANEL_H_
