/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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

#ifndef __F_REQUESTERS_H__
#define __F_REQUESTERS_H__

#include <stdarg.h>

#include <vector>
#include <string>

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

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class AlertView : public View
{
public:
    AlertView( const std::string& cText, va_list pButtons );

    virtual Point GetPreferredSize( bool bLargest );
    virtual void	 Paint( const Rect& cUpdateRect );
    virtual void	 AllAttached();
  
private:
    friend class Alert;
  
    std::vector<std::string> m_cLines;
    std::vector<Button*> 	   m_cButtons;
    Point		   m_cButtonSize;
    float		   m_vLineHeight;
    Point		   m_cMinSize;
};

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Alert : public Window
{
public:
    Alert( const std::string& cTitle, const std::string& cText, int nFlags, ... );
    ~Alert();
  
    virtual void	HandleMessage( Message* pcMessage );
  
    int  Go();
    void Go( Invoker* pcInvoker );
private:
    AlertView* m_pcView;
    Invoker*   m_pcInvoker;
    port_id    m_hMsgPort;
};


class ProgressView : public View
{
public:
    ProgressView( const Rect& cFrame, bool bCanSkip );
    void	Layout( const Rect& cBounds );

    virtual void	Paint( const Rect& cUpdateRect );
    virtual void	FrameSized( const Point& cDelta );
  
private:
    friend class ProgressRequester;
    StringView* m_pcPathName;
    StringView* m_pcFileName;
    Button*     m_pcCancel;
    Button*     m_pcSkip;
};

class ProgressRequester : public Window
{
public:
    enum { IDC_CANCEL = 1, IDC_SKIP };
  
    ProgressRequester( const Rect& cFrame, const char* pzName, const char* pzTitle, bool bCanSkip );

    virtual void	HandleMessage( Message* pcMessage );
  
    void SetPathName( const char* pzString );
    void SetFileName( const char* pzString );

    bool DoCancel() const;
    bool DoSkip();

private:
    ProgressView*	m_pcProgView;
    volatile bool m_bDoCancel;
    volatile bool m_bDoSkip;
};

}
#endif // __F_REQUESTERS_H__
