#include "imagebutton.h"

LoginImageButton::LoginImageButton(const Rect& cRect, const String& cName, const String& cLabel, Message* pcMessage, Image* pcImage, uint32 nTextPosition, bool bFrames, bool bText, bool bMouseOver) : ImageButton(cRect,cName,cLabel,pcMessage,pcImage, nTextPosition,bFrames,bText,bMouseOver,CF_FOLLOW_LEFT|CF_FOLLOW_TOP,WID_TRANSPARENT | WID_FULL_UPDATE_ON_RESIZE)
{
	cImageSize = GetImage()->GetSize();
}

LoginImageButton::~LoginImageButton()
{
}

void LoginImageButton::MouseMove(const Point &cNewPos, int nCode, uint32 nButtons, Message* pcData)
{
	ImageButton::MouseMove(cNewPos,nCode,nButtons,pcData);
}

void LoginImageButton::MouseUp( const Point& cPosition, uint32 nButton, Message* pcData )
{
	ImageButton::MouseUp(cPosition,nButton,pcData);
}

void LoginImageButton::Paint( const Rect &cUpdateRect )
{
	SetDrawingMode(DM_BLEND);
	BitmapImage* pcImage = (BitmapImage*)GetImage();
	pcImage->Draw(Point(0,0),this);	
}   

Point LoginImageButton::GetPreferredSize( bool bLargest ) const
{

	return (cImageSize);
} 
