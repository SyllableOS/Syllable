// Whisper -:-  (C)opyright 2001-2002 Kristian Van Der Vliet
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

#ifndef __IMAGE_BUTTON_H_
#define __IMAGE_BUTTON_H_

#include <gui/button.h>
#include <gui/bitmap.h>

using namespace os;

class ImageButton : public Button
{
	public:
		ImageButton(Rect cFrame, const char* pzName, const char* pzLabel, Message* pcMessage);
		~ImageButton();
		void Paint(const Rect &cUpdateRect);
		virtual void MouseMove(const Point &cNewPos, int nCode, uint32 nButtons, Message* pcData);
		void SetBitmap(uint8* pnData, uint nNumBytes);

	private:
		Bitmap* pcButtonImage;

		bool bMouseOver;
};

#endif

