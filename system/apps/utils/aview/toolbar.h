//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __F__GUI_TOOLBAR_H
#define __F__GUI_TOOLBAR_H

#include <gui/layoutview.h>
#include <gui/view.h>

namespace os
{

/** ToolBar gui element...
 * \ingroup gui
 * \par Description:
 *  Initialize the toolbar...
 * \sa
 * \author	Rick Caudill ( cau0730@cup.edu)
 *****************************************************************************/
class ToolBar : public LayoutView
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
    ToolBar(const Rect& cRect, const char* zName,bool bDrawLines=true,uint32 nPos=Horizontal, uint32 nResizeMask=CF_FOLLOW_LEFT | CF_FOLLOW_TOP | CF_FOLLOW_RIGHT, uint32 nFlags= WID_FULL_UPDATE_ON_RESIZE | WID_WILL_DRAW);
	~ToolBar();

    virtual void Paint(const Rect& cRect);
   	virtual void Draw();
	virtual void  AddNode(LayoutNode*);
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
	LayoutNode* pcNode;
};
}
#endif












