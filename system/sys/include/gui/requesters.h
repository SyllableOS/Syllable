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

#ifndef __F_GUI_REQUESTERS_H__
#define __F_GUI_REQUESTERS_H__

#include <stdarg.h>

#include <vector>
#include <gui/image.h>
#include <util/string.h>
#include <gui/window.h>
#include <gui/bitmap.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <util/message.h>

namespace os
{
#if 0
} // Fool Emacs auto-indent
#endif



class Invoker;

class AlertView : public View
{
public:
    AlertView( const String& cText, va_list pButtons, Bitmap* pcBitmap = NULL );
    //AlertView( const String, cText, va_list pButtons, BitmapImage pcImage=NULL);
    virtual ~AlertView();
    virtual Point GetPreferredSize( bool bLargest );
    virtual void	 Paint( const Rect& cUpdateRect );
    virtual void	 AllAttached();
  
private:
    AlertView& operator=( const AlertView& );
    AlertView( const AlertView& );

    friend class Alert;
    class Private;
    Private *m; 
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx), with modifications by Rick Caudill ( cau0730@cup.edu)
 *****************************************************************************/

class Alert : public Window
{
public:
	/** Icons
	 *\par: Description:
	 *    	These values are used to specify different icons in the Alert. */
	enum alert_icon{
			/** Warning Icon:  Use this when you want to flag an error Alert. */ 
		ALERT_WARNING = 0,
			/** Info Icon:     Use this when you want to flag information to the user. */  
		ALERT_INFO = 1,
			/** Question Icon: Use this when you want to ask the user a question. */ 
		ALERT_QUESTION = 2,
			/** Tip Icon:      Use this when you want to give the user a tip for something. */ 
		ALERT_TIP = 3
	}; 
 
public:
    Alert( const String& cTitle, const String& cText, int nFlags, ... );
    Alert( const String& cTitle, const String& cText, Bitmap* pcBitmap, int nFlags, ... ); 
    Alert( const String& cTitle, const String& cText, alert_icon nAlertNum, int nFlags, ...);
    //Alert( const String& cTitle, const String& cText, BitmapImage* pcBitmap, int nFlags, ... ); 
    Alert( const String& cTitle,View*);
   ~Alert();
  
   
	virtual void	HandleMessage( Message* pcMessage );
	int  Go();
	void Go( Invoker* pcInvoker );
private:
    Alert& operator=( const Alert& );
    Alert( const Alert& );

	class Private;
	Private *m;

    void SetImage(uint32 nWidth, uint32 nHeight, uint8* pnBuffer);
};


class ProgressView : public View
{
public:
    ProgressView( const Rect& cFrame, bool bCanSkip );
    virtual ~ProgressView();

    void	Layout( const Rect& cBounds );

    virtual void	Paint( const Rect& cUpdateRect );
    virtual void	FrameSized( const Point& cDelta );
  
private:
    ProgressView& operator=( const ProgressView& );
    ProgressView( const ProgressView& );

    friend class ProgressRequester;
	class Private;
	Private *m;
};

class ProgressRequester : public Window
{
public:
    enum { IDC_CANCEL = 1, IDC_SKIP };
  
    ProgressRequester( const Rect& cFrame, const String& cName, const String& cTitle, bool bCanSkip );
    virtual ~ProgressRequester();

    virtual void	HandleMessage( Message* pcMessage );
  
    void SetPathName( const String& cString );
    void SetFileName( const String& cString );

    bool DoCancel() const;
    bool DoSkip();

private:
    ProgressRequester& operator=( const ProgressRequester& );
    ProgressRequester( const ProgressRequester& );

	class Private;
	Private *m;
};

}
#endif // __F_GUI_REQUESTERS_H__
