//
// blendablecolor.h
//
// An extension of the Color32_s class, that adds blending methods.
// This class is part of the ALib Toolkit.
//
// V1.0 2002-06-07
// Copyright (C)2002 BoingSoft, AtheOS Division
//
// http://www.boing.nu/atheos
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of version 2 of the GNU Library
// General Public License as published by the Free Software
// Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free
// Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
// MA 02111-1307, USA
//
#ifndef BLENDABLECOLOR_H
#define BLENDABLECOLOR_H

#include <gui/gfxtypes.h>

//
// BlendableColor adds the following methods to the Color32_s structure:
//
// Blend(col, alpha=0.5)
// BlendAndCopy(col, alpha=0.5)
//
// Blend the colour to 'col' using 'alpha'. An 'alpha' of 1 will make the colour
// exactly like 'col', an alpha of 0 will not change the colour at all. 0.5 will result
// in a 50% blending.
// BlendAndCopy returns the result of the blending without changing the currect object.
//
//
// Gamma(y)
//
// Adjust the gamma of the colour. This can be used to convert the colour to greyscale,
// or just make a little less colourful.
// 1 = no change, 0 = black/white.
//

struct BlendableColor : public os::Color32_s
{
	BlendableColor() {}

	BlendableColor(const os::Color32_s &c)
	{
		red = c.red;
		green = c.green;
		blue = c.blue;
		alpha = c.alpha;
	}

	BlendableColor(int r, int g, int b, int a=255)
	{
		red = r; green = g; blue=b; alpha=a;
	}

	void Blend(const os::Color32_s &c, float alpha = 0.5)
	{
		red = (uint8) (red * (1 - alpha) + c.red * alpha);
		green = (uint8) (green * (1 - alpha) + c.green * alpha);
		blue = (uint8) (blue * (1 - alpha) + c.blue * alpha);
		alpha = (uint8) (alpha * (1 - alpha) + c.alpha * alpha);
	}

	BlendableColor BlendAndCopy(const os::Color32_s &c, float alpha = 0.5)
	{
		BlendableColor copy;
		copy.red = (uint8) (red * (1 - alpha) + c.red * alpha);
		copy.green = (uint8) (green * (1 - alpha) + c.green * alpha);
		copy.blue = (uint8) (blue * (1 - alpha) + c.blue * alpha);
		copy.alpha = (uint8) (alpha * (1 - alpha) + c.alpha * alpha);
		return copy;
	}

	void Gamma(float y)
	{
		int average = (red + green + blue) / 3;
		red = (uint8) (red * y + average * (1 - y));
		green = (uint8) (green * y + average * (1 - y));
		blue = (uint8) (blue * y + average * (1 - y));
	}
};

#endif /* BLENDABLECOLOR_H */







