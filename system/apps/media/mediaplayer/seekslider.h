//  Coldfish 1.0 -:-  (C)opyright 2003 Kristian Van Der Vliet
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __COLDFISH_LCD_H_
#define __COLDFISH_LCD_H_

#include <gui/view.h>
#include <gui/font.h>
#include <gui/control.h>

namespace os
{

	class SeekSlider:public Control
	{
	      public:
		SeekSlider( const Rect & cFrame, os::String zName, Message* pcMessage, uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
		 ~SeekSlider();

		void Paint( const Rect & cUpdateRect );
		void MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );

		void PostValueChange( const Variant & cNewValue );
		void EnableStatusChanged( bool bIsEnabled );

	};

}

#endif				// __SEEKSLIDER_H_
