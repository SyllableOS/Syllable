#include "colorbutton.h"

/*
** name:       ColorButton Constructor
** purpose:    Constructs the colorbutton
** parameters: Rect(that tells the size of the button), String(holds the name of the button),
**			   String(holds the title of the button), Message(holds the message of the button),
**			   Color32_s(holds the color displayed on the button)
** returns:
*/
ColorButton::ColorButton(const Rect& cFrame, const String& cName, Message* pcMessage, Color32_s sColor) : Button(cFrame,cName,"",pcMessage)
{
	sColorBg = sColor;
}

void ColorButton::SetColor(Color32_s sColor)
{
	sColorBg = sColor;
	Flush();
}

void ColorButton::Paint(const Rect& cUpdateRect)
{
	uint32 nFrameFlags = FRAME_RECESSED;
	DrawFrame(GetBounds(), nFrameFlags );
	
	View::FillRect(Rect(5,5,GetBounds().Width()-5,GetBounds().Height()-5),sColorBg);
}

Point ColorButton::GetPreferredSize( bool bLargest ) const
{
	if( bLargest )
	{
		return Point( COORD_MAX, COORD_MAX );
	} else {
		font_height sHeight;
		GetFontHeight( &sHeight );
		Point cStringExt = GetTextExtent( GetLabel() );
		return Point( cStringExt.x+35, cStringExt.y + 10 + sHeight.line_gap );
	}
}



