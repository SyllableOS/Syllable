
#include "scaledbitmap.h"

ScaledBitmap::ScaledBitmap(Bitmap *bmap, int w, int h)
	:Bitmap(w, h, CS_RGB32)
{
	ScaleBitmap(bmap);
}

void ScaledBitmap::ScaleBitmap(Bitmap *bmap)
{
	Rect	srcbounds = bmap->GetBounds();
	Rect	dstbounds = GetBounds();
	float	deltax = (srcbounds.Width()+1)/(dstbounds.Width()+1);
	float	deltay = srcbounds.Height()/dstbounds.Height();
	float	srcx, srcy = 0;
	float	dstx, dsty = 0;
	uint32	*destptr, *srcptr;
	int	dstwidth = dstbounds.Width()+1;
	int	srcwidth = srcbounds.Width()+1;

	//printf("Delta X, Y = %f, %f\n", deltax, deltay);
	//printf("Source W, H = %f, %f\n", srcbounds.Width(), srcbounds.Height());
	//printf("Destination W, H = %f, %f\n", dstbounds.Width(), dstbounds.Height());

	destptr = (uint32 *)LockRaster();
	srcptr = (uint32 *)bmap->LockRaster();

	while(dsty < dstbounds.Height()) {
		dstx = srcx = 0;
		while(dstx < dstwidth) {
			destptr[(int)dstx + (int)(dsty) * dstwidth] =
				srcptr[(int)srcx + (int)(srcy) * srcwidth];
			dstx++;
			srcx+=deltax;
		}
		dsty++;
		srcy+=deltay;
	}
	UnlockRaster();
	bmap->UnlockRaster();
}













