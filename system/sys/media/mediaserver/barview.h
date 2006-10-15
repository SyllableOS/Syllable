/*  CPUMon - Small AtheOS utility for displaying the CPU load.
 *  Copyright (C) 2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __F_BARVIEW_H__
#define __F_BARVIEW_H__

#include <util/string.h>
#include <gui/view.h>
#include <gui/font.h>
#include "mediaserver.h"

#define MESSAGE_STREAM_VOLUME_CHANGED 100

class BarView : public os::View
{
public:
    BarView( const os::Rect& cFrame );

	void	SetStreamActive( int nNum, bool bActive );
	void	SetStreamLabel( int nNum, os::String zLabel );
    void    SetStreamVolume( int nNum, int nVolume );
	
	virtual void MouseMove( const os::Point& cNewPos, int nCode, uint32 nButtons, os::Message* pcData );
    virtual void MouseDown( const os::Point& cNewPos, uint32 nButtons );
    virtual os::Point GetPreferredSize( bool bLargest ) const;
    virtual void      Paint( const os::Rect& cUpdateRect );
    
private:
    os::Rect GetBarRect( int nCol, int nBar );
    void DrawBar( int nIndex );
    
    bool             m_bStreamActive[MEDIA_MAX_AUDIO_STREAMS];
    os::String       m_zStreamLabel[MEDIA_MAX_AUDIO_STREAMS];
    int              m_anNumLighted[MEDIA_MAX_AUDIO_STREAMS];
    int				 m_anSliderPos[MEDIA_MAX_AUDIO_STREAMS];
    float            m_vNumWidth;
    os::font_height  m_sFontHeight;
};






#endif // __F_BARVIEW_H__







