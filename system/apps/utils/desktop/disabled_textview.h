#ifndef DISABLED_TEXTVIEW_H
#define DISABLED_TEXTVIEW_H

#include <gui/textview.h>
using namespace os;

class DisabledTextView : public TextView
{

    private:
        //virtual void Paint(const Rect& cUpdateRect);
        Color32_s sTextColor, sBgColor;

    public:
        DisabledTextView(const Rect& cFrame, const char* zTitle, const char* zText, bool bRead);
        DisabledTextView(const Rect& cFrame, const char* zTitle, const char* zText);
        /*	  void SetBgColor(Color32_s c)
            	{
                sBgColor.red = c.red;
                sBgColor.green = c.green;
                sBgColor.blue = c.blue;
                sBgColor.alpha = c.alpha;
            	}
            
             void SetFgColor(Color32_s c)
            	{
                sTextColor.red = c.red;
                sTextColor.green = c.green;
                sTextColor.blue = c.blue;
                sTextColor.alpha = c.alpha;
            	}*/
};

#endif



