// Eyes -:-  (C)opyright 2005 Jonas Jarvoll
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


#ifndef __EYEWIDGET_H__
#define __EYEWIDGET_H__

#include <gui/view.h>
#include <gui/point.h>
#include <gui/rect.h>
#include <gui/desktop.h>
#include <gui/menu.h>

class EyeWidget : public os::View
{
	public:
		EyeWidget( const os::String& cName,
				uint32 nResizeMask = os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP,
				uint32 nFlags = os::WID_WILL_DRAW | os::WID_CLEAR_BACKGROUND );

		~EyeWidget();

		void Paint( const os::Rect& cUpdateRect );
		void FrameSized( const os::Point& cDelta );
		void MouseDown(const os::Point& cPosition, uint32 nButtons );
		void HandleMessage(os::Message* pcMessage);
		void AttachedToWindow(void);
		os::Point GetPreferredSize(bool bLargest=false) const;
		void UpdateEyes();

	private:

		os::Menu* pcContextMenu;

		os::Point m_Mouse;
		os::Point m_MouseOld;

		os::Point CalculatePupil(os::Point orgin, os::Point mouse, os::Point eyeradius);

		void DrawCircle(float xc, float yc, float radius);
		void DrawFillCircle(float xc, float yc, float radius);

		void DrawCircle(os::Point p, float radius)  { DrawCircle( p.x, p.y, radius); };
		void DrawFillCircle(os::Point p, float radius)  { DrawFillCircle( p.x, p.y, radius); };

		void DrawEllipse(float xc, float yc, float xr, float yr);
		void DrawFillEllipse(float xc, float yc, float xr, float yr);
		inline void DrawPixel(float x, float y);
};

#endif
