#include "disabled_textview.h"

DisabledTextView::DisabledTextView(const Rect& cFrame, const char* zTitle, const char* zText, bool bRead = false) : TextView(cFrame, zTitle, zText)
{
	SetReadOnly(bRead);
	//Paint(GetBounds());
}


DisabledTextView::DisabledTextView(const Rect& cFrame, const char* zTitle, const char* zText) : TextView(cFrame, zTitle, zText)
{
	SetReadOnly(true);
	//Paint(GetBounds());
}

/*void DisabledTextView::Paint(const Rect& cUpdateRect)
{
	//FillRect(Rect(1.0f,1.0f,GetBounds().Width(),GetBounds().Height()),get_default_color(COL_NORMAL));
	
}*/




