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

#ifndef _GRADIENT_H_
#define _GRADIENT_H_

#include <atheos/types.h>
#include "HSV.h"

class Gradient
{
 public:
   os::Color32_s *color;
   uint8 steps;
 public:
   Gradient(os::Color32_s begin,os::Color32_s end,uint8 steps);
   virtual ~Gradient();   
};


#endif // _GRADIENT_H_
