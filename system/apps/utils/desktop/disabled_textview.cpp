#include "disabled_textview.h"


/*
** name:       DisabledTextView Constructor
** purpose:    Used to set readonly on a textview.  It will also change the colors of
**			   the textview(when I can figure it out)
** parameters: Rect(size and placement of the textview), const char* (title of the textview), 
**			   const char* (what is placed into the textview at initialization), bool(used to tell the
**			   textview whether to be readonly or not)
** returns:   
*/
DisabledTextView::DisabledTextView(const Rect& cFrame, const char* zTitle, const char* zText, bool bRead = false) : TextView(cFrame, zTitle, zText)
{
    SetReadOnly(bRead);
    SetEnable(bRead);
}

/*
** name:       DisabledTextView Constructor
** purpose:    Used to set readonly on a textview.  It will also change the colors of
**			   the textview(when I can figure it out)
** parameters: Rect(size and placement of the textview), const char* (title of the textview), 
**			   const char* (what is placed into the textview at initialization)
** returns:   
*/
DisabledTextView::DisabledTextView(const Rect& cFrame, const char* zTitle, const char* zText) : TextView(cFrame, zTitle, zText)
{
    SetReadOnly(true);
}





