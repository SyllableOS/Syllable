#ifndef _LOGIN_IMAGE_VIEW_H
#define _LOGIN_IMAGE_VIEW_H

#include <gui/imageview.h>
#include <gui/view.h>

#include "commonfuncs.h"

using namespace os;

class LoginImageView : public View
{
public:
	LoginImageView(const Rect&);
	~LoginImageView();
	
	void Paint(const Rect&);
	void ResizeBackground();
private:
	BitmapImage* pcImage;
	
};

#endif	


