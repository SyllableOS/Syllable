//-----------------------------------------------------------------------------
#include <stdio.h>
//-------------------------------------
#include <gui/bitmap.h>
//-------------------------------------
//-------------------------------------
#include "bitmapview.h"
//-----------------------------------------------------------------------------

BitmapView::BitmapView( os::Point size, uint32 resizemask ) :
	os::View( os::Rect(0,0,0,0), "Bitmap view", resizemask, os::WID_FULL_UPDATE_ON_RESIZE )
{
    fSize = size;
//    SetViewColor( B_TRANSPARENT_COLOR );
    fBitmap = NULL;
}

//-----------------------------------------------------------------------------

void BitmapView::Paint( const os::Rect &rect )
{
//    printf( "%f %f\n", rect.Width(), rect.Height() );
    if( fBitmap )
	DrawBitmap( const_cast<os::Bitmap*>(fBitmap), fBitmap->GetBounds(), GetBounds() );
    else
	FillRect( rect );
}

os::Point BitmapView::GetPreferredSize( bool max ) const
{
    return fSize;
}

//-----------------------------------------------------------------------------

void BitmapView::DrawNewBitmap( const os::Bitmap *bitmap )
{
    fBitmap = bitmap;

    if( fBitmap )
    {
	Invalidate();
	Flush();
    }
}

//-----------------------------------------------------------------------------
