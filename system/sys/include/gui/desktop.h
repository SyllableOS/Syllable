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

#ifndef __F_GUI_DESKTOP_H__
#define __F_GUI_DESKTOP_H__

#include <atheos/types.h>

#include <gui/region.h>
#include <gui/guidefines.h>

namespace os {
#if 0
} // Fool Emacs auto-indent
#endif

/** Class for manipulating the 32 desktops.
 * \ingroup gui
 * \par Description:
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Desktop
{
public:
    enum { ACTIVE_DESKTOP = 9999 };

    Desktop( int nDesktop = ACTIVE_DESKTOP );
    ~Desktop();

    screen_mode GetScreenMode() const;
    IPoint	GetResolution() const;
    color_space	GetColorSpace() const;
    void*	GetFrameBuffer();
    bool	SetScreenMode( screen_mode* psMode );
    bool	SetResoulution( int nWidth, int nHeight );
    bool	SetColorSpace( color_space eColorSpace );
    bool	SetRefreshRate( float vRefreshRate );
    bool	Activate();
  
private:
    int		 m_nCookie;
    int		 m_nDesktop;
    screen_mode* m_psScreenMode;
    void*	 m_pFrameBuffer;
    uint32	 unused[6];

    area_id	m_hServerAreaID;
    area_id	m_hLocalAreaID;
};

}

#endif // __F_GUI_DESKTOP_H__

