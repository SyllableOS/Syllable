#ifndef HSV_H_
#define HSV_H_

#include <gui/gfxtypes.h>
#include <math.h>

struct HSV
{
   float hue;
   float saturation;
   float value;
 public:
   HSV(float h,float s,float v)
   {
      hue = h;
      while (hue < 0)
	hue += 360;
      while (hue > 360)
	hue -= 360;
      saturation = s;
      value = v;
   }
   HSV(os::Color32_s& c)
   {
      float min = (c.red < c.green)?((c.red < c.blue)? c.red : c.blue): (c.green < c.blue)? c.green: c.blue;
      float max = (c.red > c.green)?((c.red > c.blue)? c.red : c.blue): (c.green > c.blue)? c.green: c.blue;
      value = max;
      float delta = max - min;
      if (max != 0)
	saturation = delta/max;
      else
      {
	 saturation = 0;
	 hue = -1;
	 return;
      }
      if (delta == 0)
      {
	 hue = -1;
	 return;
      }
      else if (c.red==max)
	hue = (c.green - c.blue) / delta;
      else if (c.green == max)
	hue = 2 + (c.blue - c.red) / delta;
      else
	hue = 4 + (c.red - c.green) / delta;
      hue *=60;
      if (hue < 0)
        hue +=360;
   }
  os::Color32_s GetRGB()
   {
      int i;
      float f,p,q,t;
      if (saturation == 0)
      {//achromatic
	 return os::Color32_s((int)value,(int)value,(int)value);
      }
      float h = hue/60;
      i = (int)floor(h);
      f = h - i;
      p = value * (1 - saturation);
      q = value * (1 - saturation * f);
      t = value * (1 - saturation * (1 - f));
      switch(i)
      {
       case 0:
	 return os::Color32_s((int)value,(int)t,(int)p);
       case 1:
	 return os::Color32_s((int)q,(int)value,(int)p);
       case 2:
	 return os::Color32_s((int)p,(int)value,(int)t);
       case 3:
	 return os::Color32_s((int)p,(int)q,(int)value);
       case 4:
	 return os::Color32_s((int)t,(int)p,(int)value);
       default:
	 return os::Color32_s((int)value,(int)p,(int)q);
      }
      
   }
};
#endif // HSV_H_

