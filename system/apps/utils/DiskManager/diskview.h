/*
 *  DiskManager - AtheOS disk partitioning tool.
 *  Copyright (C) 2000 Kurt Skauen
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


#include <gui/layoutview.h>
#include "main.h"
/*
class Disk
{
    std::string& 	m_cDevicePath;
    device_geometry	m_cGeometry;
};
*/

namespace os
{
    class Button;
    class ListView;
};

class DiskView : public os::LayoutView
{
public:
    DiskView( const os::Rect& cFrame, const std::string& cName );
    
    virtual void	AllAttached();
    virtual void	Paint( const os::Rect& cUpdateRect );
  
    virtual void 	HandleMessage( os::Message* pcMessage );
    
    virtual void	MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
    virtual void	MouseDown( const os::Point& cPosition, uint32 nButtons );
    virtual void	MouseUp( const os::Point& cPosition, uint32 nButtons, os::Message* pcData );
    virtual void	KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void	KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    
private:
    os::ListView*	   m_pcDiskList;
    os::Button*		   m_pcPartitionButton;
    os::Button*		   m_pcQuitButton;
};
