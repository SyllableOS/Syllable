/*
 * Albert 0.4 * Copyright (C)2002 Henrik Isaksson
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "togglebutton.h"

ToggleButton::ToggleButton(const Rect& cFrame, char *x, char *y, Message *z)
	:Button(cFrame, x, y, z)
{

}

ToggleButton::~ToggleButton()
{
}

void ToggleButton::Paint(const Rect &update)
{
//	Rect cBounds = GetBounds();
	Button::Paint(update);
/*	if(m_Down)
		DrawFrame(cBounds, FRAME_RECESSED);
	else
		DrawFrame(cBounds, FRAME_RECESSED);

	SetFgColor(0,0,0,0);
	SetBgColor(get_default_color(COL_NORMAL));

	char *text = m_Contents;
	if(!text) text = "0";

	font_height fh;
	GetFontHeight(&fh);
	float fontheight = fh.ascender + fh.descender;
	float baseline = cBounds.top + (cBounds.Height() + 1)/2 - fontheight/2 + fh.ascender;
	float tw = GetStringWidth(text);

	MovePenTo(cBounds.right - 3.0f - tw, baseline);
	DrawString(text);
*/
}
	
void ToggleButton::MouseUp( const Point& cPosition, uint32 nButton, Message* pcData )
{
	View::MouseUp( cPosition, nButton, pcData );
}

void ToggleButton::MouseDown( const Point& cPosition, uint32 nButton )
{
	bool state = GetValue().AsBool();

	if ( nButton != 1 || IsEnabled() == false ) {
		View::MouseDown( cPosition, nButton );
		return;
	}

	/*if(!GetBounds().DoIntersect( cPosition ))
		return;*/

	SetValue(!state, true);
}


