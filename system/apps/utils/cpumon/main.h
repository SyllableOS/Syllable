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

#ifndef __F_MAIN_H__
#define __F_MAIN_H__

#include <util/application.h>
#include <gui/window.h>

namespace os {
    class View;
};

class MonWindow : public os::Window
{
public:
    MonWindow( const os::Rect& cFrame );
    ~MonWindow();

    bool	OkToQuit();
  
private:
    os::View* 	m_pcLoadView;
    int		m_nCPUCount;
};

class	MyApp : public os::Application
{
public:
    MyApp();
    ~MyApp();
private:
    MonWindow* m_pcWindow;
};


void GetLoad( int nCPUCount, float* pavLoads );


#endif // __F_MAIN_H__


