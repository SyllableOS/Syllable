/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 Syllable Team
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

#ifndef	__F_GUI_PROGRESSBAR_H__
#define	__F_GUI_PROGRESSBAR_H__

#include <gui/view.h>


namespace os
{
#if 0	// Fool Emacs auto-indent
}
#endif

/** 
 * \ingroup gui
 * \par Description:
 *	A ProgressBar is a way to show how much progress is happening in certain situations.
 *	A ProgressBar is useful when you are doing large file copying, browsing a website, or
 *	anything that can a bit of time to do.  It is a great way to keep the 'user' informed
 *	when something is happening.
 * \par The best way to use the ProgressBar is as follows
 * \code
		os::ProgressBar pcProgressBar = new os::ProgressBar(cProgressBarFrame,"Progress");
		
		...
		//SetProgress takes a float between 0.0(i.e., when the ProgressBar is empty) and 1.0(i.e., when the ProgessBar is full)
		pcProgressBar->SetProgress(0.1);
 * \sa os::Rect os::String os::orientation os::View
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class ProgressBar : public View
{
public:
    ProgressBar( const Rect& cFrame, const String& cTitle, orientation eOrientation = HORIZONTAL,
	  uint32 nResizeMask = CF_FOLLOW_LEFT | CF_FOLLOW_TOP,
	  uint32 nFlags = WID_WILL_DRAW | WID_CLEAR_BACKGROUND );
	  
	virtual ~ProgressBar();

    void SetProgress( float vValue );
    float GetProgress() const;

    virtual Point GetPreferredSize( bool bLargest ) const;

    virtual void AttachedToWindow();
    virtual void Paint( const Rect& cUpdateRect );
    virtual void FrameSized( const Point& cDelta );
    
private:
    virtual void	__PB_reserved1__();
    virtual void	__PB_reserved2__();
    virtual void	__PB_reserved3__();
    virtual void	__PB_reserved4__();
    virtual void	__PB_reserved5__();

private:
    ProgressBar& operator=( const ProgressBar& );
    ProgressBar( const ProgressBar& );

	class Private;
	Private *m;
};



} // end of namespace

#endif // __F_GUI_PROGRESSBAR_H__
