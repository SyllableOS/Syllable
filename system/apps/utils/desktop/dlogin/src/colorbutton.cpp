#include "colorbutton.h"
#include <gui/font.h>
#include <math.h>

/*
** name:       ColorButton Constructor
** purpose:    Constructs the colorbutton
** parameters: Rect(that tells the size of the button), char*(holds the name of the button),
**			   char*(holds the title of the button), Message(holds the message of the button),
**			   Color32_s(holds the background color of the button), Color32_s(holds
**			   the foreground color of the button).
** returns:
*/
ColorButton::ColorButton(const Rect& cFrame, char* s1, char* s2,Message* pcMess, Color32_s bColor, Color32_s fColor) : Button(cFrame, s1, s2, pcMess)
{
    SetBgColor(bColor);
    SetFgColor(fColor);
    Paint(GetBounds());
}


/*
** name:       ColorButton Constructor
** purpose:    Constructs the colorbutton
** parameters: Rect(that tells the size of the button), char*(holds the name of the button),
**			   char*(holds the title of the button), Message(holds the message of the button),
**			   Color32_s(holds the background color of the button).
** returns:
*/
ColorButton::ColorButton(const Rect& cFrame, char* s1, char* s2,Message* pcMess, Color32_s bColor) : Button(cFrame, s1, s2, pcMess)
{
    Color32_s sColor(0,0,0,255);
    SetFgColor(sColor);
    SetBgColor(bColor);
    Paint(GetBounds());
}

/*
** name:       ColorButton::Paint
** purpose:    Paint
** parameters: Rect of the button.
** returns:
*/
void ColorButton::Paint(const Rect & cUpdateRect)
{
	Rect cBounds = GetBounds();
	View::SetEraseColor( sBgColor);
	uint32 nFrameFlags = ( GetValue().AsBool(  ) && IsEnabled(  ) )? FRAME_RECESSED : FRAME_RAISED;

	if( IsEnabled() && (HasFocus()) )
	{
		if( HasFocus() ) {
			View::SetFgColor( 0, 0xAA, 0 );
		} else {
			View::SetFgColor( 0, 0, 0 );
		}
		DrawLine( Point( cBounds.left, cBounds.top ), Point( cBounds.right, cBounds.top ) );
		DrawLine( Point( cBounds.right, cBounds.bottom ) );
		DrawLine( Point( cBounds.left, cBounds.bottom ) );
		DrawLine( Point( cBounds.left, cBounds.top ) );
		cBounds.Resize( 1, 1, -1, -1 );
	}

	DrawFrame( cBounds, nFrameFlags );

	if( IsEnabled() )
	{
		View::SetFgColor( sTextColor );
	}
	else
	{
		View::SetFgColor( 255, 255, 255 );
	}
	View::SetBgColor(sBgColor);

	if( GetValue().AsBool(  ) )
	{
		cBounds.MoveTo( 1, 1 );
	}

	DrawText( cBounds, GetLabel(), DTF_ALIGN_CENTER );
	if( IsEnabled() == false )
	{
		View::SetFgColor( 100, 100, 100 );
		SetDrawingMode( DM_OVER );
		cBounds.MoveTo( -1, -1 );
		DrawText( cBounds, GetLabel(), DTF_ALIGN_CENTER );
		SetDrawingMode( DM_COPY );
	}
}





