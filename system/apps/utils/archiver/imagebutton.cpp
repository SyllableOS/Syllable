//  Archiver 0.3.0 -:-  (C)opyright 2000-2001 Rick Caudill
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Code based on Vanders' Aedit imagebutton code


#include <gui/button.h>
#include <gui/bitmap.h>
#include <stdio.h>

using namespace os;

class ImageButton : public Button
{
public:
    ImageButton(Rect cFrame, const char* pzName, const char* pzLabel, Message* pcMessage);
    void Paint(const Rect &cUpdateRect);
    void SetBitmap(uint8* pData, uint iNumBytes);

private:
    Bitmap* pcButtonImage;
};

ImageButton::ImageButton(Rect cFrame, const char* pzName, const char* pzLabel, Message* pcMessage) : Button(cFrame, pzName, pzLabel, pcMessage, CF_FOLLOW_LEFT, WID_WILL_DRAW | WID_FULL_UPDATE_ON_RESIZE)
{
    pcButtonImage=new Bitmap(cFrame.Width(), cFrame.Height(), CS_RGB32, 0x0002);
}

void ImageButton::Paint(const Rect &cUpdateRect)
{
    Rect cBounds = GetBounds();

    SetEraseColor( get_default_color( COL_NORMAL ) );

    if ( GetWindow()->GetDefaultButton() == this ) {
        SetFgColor( 0, 0, 0 );
        DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
        DrawLine( Point( cBounds.right, cBounds.bottom ) );
        DrawLine( Point( cBounds.left, cBounds.bottom ) );
        DrawLine( Point( cBounds.left, cBounds.top ) );
        cBounds.left += 1;
        cBounds.top += 1;
        cBounds.right -= 1;
        cBounds.bottom -= 1;
        DrawFrame( cBounds, (GetValue()) ? FRAME_RECESSED : FRAME_RAISED );
        //SetDrawingMode(DM_BLEND);
    }
    else{
        DrawFrame( cBounds, (GetValue()) ? FRAME_RECESSED : FRAME_RAISED );
    }

    Rect cBitmapRect=pcButtonImage->GetBounds();
    cBitmapRect.top+=2;
    cBitmapRect.left+=2;
    cBitmapRect.bottom-=2;
    cBitmapRect.right-=2;
    DrawBitmap(pcButtonImage, cBitmapRect, cBitmapRect);

}

void ImageButton::SetBitmap(uint8* pData, uint iNumBytes)
{
    memcpy( pcButtonImage->LockRaster(), pData, iNumBytes);
}

