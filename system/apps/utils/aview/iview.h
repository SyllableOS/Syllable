//  AView (C)opyright 2005 Kristian Van Der Vliet
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

#include <gui/view.h>
#include <gui/imageview.h>
#include <gui/image.h>
#include <gui/scrollbar.h>
#include <gui/rect.h>
#include <gui/window.h>
#include <util/variant.h>

#include "messages.h"

#ifndef __F_AVIEW_NG_IVIEW_H_
#define __F_AVIEW_NG_IVIEW_H_

using namespace os;

class IView : public View
{
	public:
		IView( Rect cFrame, const char *pzName, Window *pcParent );
		~IView( );
		virtual void FrameSized( const Point &cDelta );
		virtual void WheelMoved( const Point &cDelta );
		virtual void KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers );
		void WindowActivated( bool bIsActive );

		void SetImage( Image *pcImage );

	private:
		ImageView *m_pcImageView;

		ScrollBar *m_pcHScrollBar;
		ScrollBar *m_pcVScrollBar;

		int32 m_nImageX, m_nImageY;

		Window *m_pcParent;
};

#define IV_SCROLL_WIDTH		14
#define IV_SCROLL_HEIGHT	14

#endif		// __F_AVIEW_NG_IVIEW_H_

