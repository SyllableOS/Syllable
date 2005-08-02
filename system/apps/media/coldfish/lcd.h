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

	class Lcd:public Control
	{
	      public:
		Lcd( const Rect & cFrame, Message* pcMessage, uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP );
		 ~Lcd();

		void Paint( const Rect & cUpdateRect );
		void MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData );

		void SetTrackName( std::string zName );
		void SetTrackNumber( int nTrack );
		void UpdateTime( uint64 nTime );
		
		void PostValueChange( const Variant & cNewValue );
		void EnableStatusChanged( bool bIsEnabled );

	      private:
		std::string m_zName;
		int m_nTrack;
		uint64 m_nTime;
		
		Font *m_pcLcdFont;
	};

}

#endif				// __COLDFISH_LCD_H_
