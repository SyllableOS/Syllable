#ifndef COLOR_BUTTON_H
#define COLOR_BUTTON_H
#include <gui/button.h>

using namespace os;

class ColorButton : public Button
{

private:
    Color32_s sTextColor, sBgColor;
    virtual void Paint(const Rect& cUpdateRect);

public:
    ColorButton(const Rect &cFrame, char* s1, char* s2, Message* pcMess, Color32_s bColor, Color32_s fColor);
    ColorButton(const Rect &cFrame, char* s1, char* s2, Message* pcMess,Color32_s bColor);
   
    void SetFgColor(int r, int g, int b)
    {
        sTextColor.red = r;
        sTextColor.green = g;
        sTextColor.blue = b;
    }

    void SetBgColor(int r, int g, int b)
    {
        sBgColor.red = r;
        sBgColor.green = g;
        sBgColor.blue = b;
    }

    void SetBgColor(Color32_s c)
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
    }
};
#endif


