/*
 *  Copyright (C) 2001 Edwin de Jonge
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <math.h>
#include "gradient.h"

Gradient::Gradient(os::Color32_s begin,os::Color32_s end,uint8 steps)
{
   this->steps = steps;
   this->color = new os::Color32_s[steps];
   
   HSV hBegin(begin);
   HSV hEnd(end);
   if (hEnd.hue < 0)
     hEnd.hue = hBegin.hue;
   if (hBegin.hue < 0)
     hBegin.hue = hEnd.hue;
   float hDelta = hEnd.hue - hBegin.hue;
   //hue is cyclic so take the short route
   if (hDelta > 180.)
     hDelta -= 360.;
   if (hDelta < -180.)
     hDelta +=360.;
   float sDelta = hEnd.saturation - hBegin.saturation;
   float vDelta = hEnd.value - hBegin.value;
   //divide in steps
   hDelta /= steps;
   sDelta /= steps;
   vDelta /= steps;
   for (int s=0; s < steps; s++)
     {
	color[s] = HSV(hBegin.hue + s*hDelta,hBegin.saturation + s*sDelta,hBegin.value + s*vDelta).GetRGB();
     }
}

Gradient::~Gradient()
{
   delete[] this->color;
}
