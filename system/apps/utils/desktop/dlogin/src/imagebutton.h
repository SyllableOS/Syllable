#ifndef _DLOGIN_IMAGE_BUTTON_H
#define _DLOGIN_IMAGE_BUTTON_H

#include <gui/imagebutton.h>
using namespace os;

class LoginImageButton : public ImageButton
{
public:
	LoginImageButton(const Rect&, const String&, const String&,Message*, Image*, uint32, bool, bool, bool);
	~LoginImageButton();

    virtual void MouseMove(const Point &cNewPos, int nCode, uint32 nButtons, Message* pcData);
    virtual void MouseUp( const Point& cPosition, uint32 nButton, Message* pcData );
    virtual void Paint( const Rect &cUpdateRect );   
    virtual Point GetPreferredSize( bool bLargest ) const;
private:
	Point cImageSize;
};
	
#endif
