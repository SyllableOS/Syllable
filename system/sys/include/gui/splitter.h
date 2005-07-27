/*  Splitter class
 *  Copyright (C) 2002  Sebastien Keim
 *  Copyright (C) 2003 - 2005  Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef	__F_GUI_SPLITTER_H__
#define	__F_GUI_SPLITTER_H__

/** \file gui/splitter.h
 * \par Description:
 *	Declaration of the os::Splitter class
 * \author Sebastien Keim (s.keim@laposte.net)
 *****************************************************************************/

#include <gui/guidefines.h>
#include <gui/view.h>

namespace os_priv{
   class SplitterSeparator;
};

namespace os
{
/** Splitter View.
 * \ingroup gui 
 *
 * An Splitter object stacks two subviews within one view so that the user can 
 * change their relative sizes. By default, the separator bar between the 
 * views is horizontal, so the views are one on top of the other. To have 
 * vertical split bars (so the views are side by side), use the method 
 * Splitter::SetDirection. The rest of this section assumes you have 
 * horizontal split bars and gives information on vertical split bars in 
 * parentheses.
 * 
 * The Splitter resizes its subviews so that each subview is the same width
 * (or height) as the Splitter view, and the total of the subviews' heights
 * (or widths), plus the separator's thicknesses, is equal to the height
 * (or width) of the Splitter. The Splitter positions its subviews so that
 * the first subview is at the top (or left) of the Splitter frame. The user
 * can set the height (or width) of two subviews by moving a horizontal
 * (or vertical) bar called the separator, which makes one subview smaller
 * and the other larger. 
 *
 * \bug
 * When you embed a splitter into another splitter, this can cause refresh
 * problems when the embeded splitter is resized (mosty when it contain text
 * views)
 * \author Sebastien Keim (s.keim@laposte.net)
 */
class Splitter : public View
{ 
public:	
	Splitter(const Rect &cFrame, const String &cTitle, View* pView1, 
		View* pView2, orientation eOrientation = HORIZONTAL,
		uint32 nResizeMask = CF_FOLLOW_LEFT|CF_FOLLOW_TOP,
		uint32 nFlags = WID_WILL_DRAW|WID_CLEAR_BACKGROUND);
   
	virtual ~Splitter();

	// From Splitter:
	virtual void SplitChanged();

	void SetSplitRatio( float vRatio );
	float GetSplitRatio() const;
   
	void SetSplitLimits(float fMinSize1=0, float fMinSize2=0);
	void SetOrientation(os::orientation eOrientation);

	View* SeparatorView() const;
	float GetSplitPosition() const;
	float GetSeparatorWidth() const;
	void  SetSeparatorWidth(float fWidth);

	void AdjustLayout();

	void SplitBy(float fValue);
	void SplitTo(float fValue);

	// From View:
	virtual void MouseMove(const Point &cNewPos, int nCode, uint32 nButtons, Message *pcData);
	virtual void MouseUp(const Point &cPosition, uint32 nButtons, Message *pcData);
	virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
	virtual void FrameSized(const Point& cDelta);

	virtual os::Point GetPreferredSize(bool bLargest) const;

private:
    virtual void	__SS_reserved1__();
    virtual void	__SS_reserved2__();
    virtual void	__SS_reserved3__();
    virtual void	__SS_reserved4__();
    virtual void	__SS_reserved5__();

private:
    Splitter& operator=( const Splitter& );
    Splitter( const Splitter& );

	void StartTracking(Point position); 
	friend class os_priv::SplitterSeparator;

	class Private;
	Private* m;
};

} //namespace os

#endif //ndef __F_GUI_SPLITTER_H__
