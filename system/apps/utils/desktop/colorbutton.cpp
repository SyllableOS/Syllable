
#include "colorbutton.h"
#include <gui/font.h>

ColorButton::ColorButton(const Rect& cFrame, char* s1, char* s2,Message* pcMess, Color32_s bColor, Color32_s fColor) : Button(cFrame, s1, s2, pcMess)
{
    SetBgColor(bColor);
    SetFgColor(fColor);
    Paint(GetBounds());
}

ColorButton::ColorButton(const Rect& cFrame, char* s1, char* s2,Message* pcMess, Color32_s bColor) : Button(cFrame, s1, s2, pcMess)
{
	Color32_s sColor(0,0,0,255);
    SetFgColor(sColor);
    SetBgColor(bColor);
    Paint(GetBounds());
}


void ColorButton::Paint(const Rect & cUpdateRect)
{

    Rect cBounds = GetNormalizedBounds();
    font_height sHeight;
    GetFontHeight( &sHeight );
    float vStrWidth = GetStringWidth( GetLabel() );
    float vCharHeight = sHeight.ascender - sHeight.descender + sHeight.line_gap;
    float y = floor((cBounds.Height()+1.0f)*0.5f - vCharHeight*0.5f + sHeight.ascender + sHeight.line_gap / 2 );
    float x = floor((cBounds.Width()+1.0f) / 2.0f - vStrWidth / 2.0f);

    if( GetValue() )
    {
        y += 1.0f;
        x += 1.0f;
    }

    SetEraseColor( sBgColor );
    DrawFrame( cBounds, (GetValue()) ? FRAME_RECESSED : FRAME_RAISED );

    View::SetFgColor( sTextColor );
    View::SetBgColor( sBgColor );
    MovePenTo( x, y );
    //SetDrawingMode(DM_OVER);
    DrawString( GetLabel() );

}




