#ifndef FRIDGECAM_BITMAPVIEW_H
#define FRIDGECAM_BITMAPVIEW_H
//-----------------------------------------------------------------------------

#include <gui/view.h>
namespace os { class Bitmap; }

//-----------------------------------------------------------------------------

class BitmapView : public os::View
{
public:
    BitmapView( os::Point size, uint32 resizeMask );

    void DrawNewBitmap( const os::Bitmap *bitmap );

protected:
    void Paint( const os::Rect &rect );

    os::Point GetPreferredSize( bool max ) const;

    virtual void SetFrame( const os::Rect& cRect, bool bNotifyServer = true )
    {
	printf( "SIZE: %fx%f\n", cRect.Width(),cRect.Height() );
	os::View::SetFrame( cRect, bNotifyServer );
    }

private:
    os::Point	     fSize;
    
    const os::Bitmap *fBitmap;
};

//-----------------------------------------------------------------------------
#endif
