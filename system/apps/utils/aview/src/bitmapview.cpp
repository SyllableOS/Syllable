#include "bitmapview.h"
#include <stdio.h>

using namespace std;
BitmapView::BitmapView(Bitmap* pcBitmap, Window* pcParent) : View( pcParent->GetBounds() , "bitmap_view", CF_FOLLOW_ALL )
{
    m_pcBitmap = pcBitmap;
}

void BitmapView::Paint( const Rect& cUpdateRect )
{
    Rect cRect = m_pcBitmap->GetBounds();
    SetEraseColor(0,0,0,0);
    SetBgColor(0,0,0);
    SetFgColor(0,0,0);
    FillRect(cUpdateRect);
    DrawBitmap( m_pcBitmap, cRect, cRect );
}

void BitmapView::KeyDown( const char* pzString , const char* pzRawString, uint32 nQualifiers)
{
    // Key down events are traped by this over-riden method.  All we do is take the message created, and pass it
    // stright back to our AppWindow::HandleMessage method.  Why?  Because AppWindow is where most of the code lives,
    // and it is simply cleaner and more logical to handle this event from HandleMessage then down here.

    Message* pcMessage = GetWindow()->GetCurrentMessage();
    GetWindow()->HandleMessage(pcMessage);
}

void BitmapView::MouseDown( const Point &nPos, uint32 nButtons )
{
    GetWindow()->FindView(nPos)->MakeFocus();
}

void BitmapView::ScrollBack(float nPosWidth, float nPosHeight)
{
    ScrollTo(-(nPosWidth), -(nPosHeight));

}

/*void BitmapView::WheelMoved(const os::Point &p)
{
    os::Point off=GetScrollOffset();
    off.y=off.y-3*p.y;
    if (off.y > m_pcBitmap->GetBounds().Height()-Height())
    	ScrollTo(off);
}*/









