// Copyright 1999 Jesper Hansen. Free for use under the Gnu Public License
// Ported to AtheOS by Kurt Skauen 29/10-1999

#ifndef DAMN_BITMAPSCALE_H
#define DAMN_BITMAPSCALE_H
//-----------------------------------------------------------------------------
//-------------------------------------
#include <sys/types.h>
#include <gui/bitmap.h>
//-------------------------------------
//-----------------------------------------------------------------------------

using namespace os;

enum bitmapscale_filtertype
{
    filter_filter,
    filter_box,
    filter_triangle,
    filter_bell,
    filter_bspline,
    filter_lanczos3,
    filter_mitchell
};

void Scale( Bitmap *srcbitmap, Bitmap *dstbitmap, bitmapscale_filtertype filtertype, float filterwidth=0.0f );

//-----------------------------------------------------------------------------
#endif

