#ifndef BITMAP_VIEW_H
#define BITMAP_VIEW_H

#include <gui/bitmap.h>
#include <gui/view.h>
#include <gui/window.h>

using namespace os;
class BitmapView : public View
{
public:
    BitmapView(Bitmap* pcBitmap, Window*);
    void Paint( const Rect& cUpdateRect );
    void KeyDown( const char* pzString , const char* pzRawString, uint32 nQualifiers);
    void MouseDown( const Point &nPos, uint32 nButtons );
    void ScrollBack(float nPosWidth, float nPosHeight);
    //virtual void WheelMoved(const os::Point&);

private:
    Bitmap* m_pcBitmap;
    View* main_bitmap_view;
};

#endif







