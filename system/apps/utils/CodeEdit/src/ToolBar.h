#ifndef __F__GUI_TOOLBAR_H
#define __F__GUI_TOOLBAR_H

#include <gui/view.h>
using namespace os;

static Rect rLastRect;

/** ToolBar gui element...
 * \ingroup gui
 * \par Description:
 *  Initialize the toolbar...
 * \sa
 * \author	Rick Caudill ( cau0730@cup.edu)
 *****************************************************************************/
class ToolBar : public View
{
public:

    /** Position types
     * \par Description:
     *      These values are used to specify the position in which the outerlines of the toolbar will 
     *      be placed.
     **/
    enum ToolBar_Position{
        /**Draws the outerlines vertically*/
        Vertical = 0x0005,
        /**Draws the outerlines horizontally*/
        Horizontal = 0x0006
    };

public:
    ToolBar(const Rect& cRect, const char* zName,bool bDrawLines=true,uint32 nPos=Horizontal, uint32 nResizeMask=CF_FOLLOW_RIGHT | CF_FOLLOW_LEFT | CF_FOLLOW_TOP, uint32 nFlags= WID_FULL_UPDATE_ON_RESIZE | WID_WILL_DRAW);
    virtual ~ToolBar();

    virtual void FrameSized(const Point& cDelta);
    virtual void Paint(const Rect& cRect);
    virtual void Draw();

    virtual void _reserved1();
    virtual void _reserved2();
    virtual void _reserved3();
    virtual void _reserved4();
    virtual void _reserved5();
    virtual void _reserved6();
    virtual void _reserved7();
    virtual void _reserved8();
    virtual void _reserved9();
    virtual void _reserved10();

private:
    bool bDraw;
    uint32 nPosition;
};

/*class ToolBarButton : public ImageButton
{
}
 
class ToolBarSeparator : public View
{
} */

#endif









