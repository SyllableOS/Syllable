/*
 *  SyllableManager - System Manager for Syllable
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

#ifndef __F_SYLLABLEPANEL_H__
#define __F_SYLLABLEPANEL_H__

#include <atheos/kernel.h>
#include <gui/view.h>
#include <gui/layoutview.h>
#include <fcntl.h>
#include <unistd.h>


using namespace os;


namespace os {
  class Button;
  class TreeView;
  class StringView;
}

class DevicesPanel : public os::LayoutView
{
public:
  
    DevicesPanel( const os::Rect& cFrame );
    ~DevicesPanel() {}

    virtual void AllAttached();
    virtual void HandleMessage( Message* pcMessage );

    void RefreshDevicesList();
   
private:
   
	Button*	m_pcRefreshBut;  
	TreeView*	m_pcDevicesList;

};

#endif // __F_DEVICESPANEL_H__



