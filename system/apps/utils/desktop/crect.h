/*
 *  CRect - Screen Centred Rect
 *  Copyright (C) 2001 Henrik Isaksson
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

#ifndef __CRECT_H
#define __CRECT_H

#include <gui/rect.h>
#include <gui/desktop.h>

using namespace os;

class CRect : public Rect {
	public:
	CRect(float w, float h) {
		float		x, y;
		Desktop	desktop;
		x = desktop.GetResolution().x/2;
		y = desktop.GetResolution().y/2;
		left = x-w/2;
		top = y-h/2;
		right = x+w/2;
		bottom = y+h/2;
	}
};

#endif /* __CRECT_H */

