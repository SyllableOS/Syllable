
#ifndef SCALED_BITMAP_H
#define SCALED_BITMAP_H

#include <gui/bitmap.h>

using namespace os;

class ScaledBitmap : public Bitmap
{
	public:
	ScaledBitmap(Bitmap *bitmap, int width, int height);
	ScaledBitmap(Bitmap *bitmap, float factor);
	
	private:
	void ScaledBitmap::ScaleBitmap(Bitmap *bmap);
};

#endif // SCALED_BITMAP_H


