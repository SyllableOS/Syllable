/*  clockview.h - the main window for the AtheOS Clock v0.3
 *  Copyright (C) 2001 Chir
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


#include <gui/window.h>
#include <gui/layoutview.h>
#include <atheos/time.h>
#include <strings.h>
#include <stdio.h>

using namespace os;

/** Clock component */
class ClockView : public LayoutView
{
    void draw( const Rect& cUpdateRect);
    bool m_bIsFirst;                            // Initialisation flag.
    int m_nHour, m_nMin, m_nSec;                // Previous time shown.
    bool m_bShowSec;                            // Whether to show seconds moving.
    bool m_bShowDigital;                        // Whether to use the digital or the analog view.
    Color32_s sBackColor;
  public:
    ClockView( Rect cFrame, Color32_s, bool, bool );
    ~ClockView()  { }
    void Paint( const Rect& cUpdateRect );
    void TimerTick( int nID );                  // virtual.
    void FrameSized( const Point& cDelta );     // virtual.
    void showSeconds( bool bShowing );
    void showDigital( bool bShowing );
    void SetBackColor(Color32_s);
    Color32_s GetBackColor();
    bool GetDigital();
	bool GetShowSeconds();

};


