/*  Media Server
 *  Copyright (C) 2003 Arno Klenke
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
 
#ifndef _MEDIACONTROLS_H_
#define _MEDIACONTROLS_H_

#include <atheos/soundcard.h>
#include "mixerchannel.h"
#include "barview.h"

namespace os
{

class MediaControls : public Window
{
public:
	MediaControls( MediaServer* pcServer, Rect cFrame );
	~MediaControls();
	
	virtual bool OkToQuit();
	virtual void HandleMessage( Message* pcMessage );
	
	void SetMasterValue( float vValue );
	void StreamChanged( uint32 nNum );
	void SetStreamValue( uint32 nNum, float vValue );
	
private:
	MediaServer*	m_pcServer;
	int 			m_hMixerDev;
	TabView*		m_pcTabs;
	MixerChannel*	m_pcMixerChannel[SOUND_MIXER_NRDEVICES];
	bool			m_bStreamActive[MEDIA_MAX_AUDIO_STREAMS];
	BarView*		m_pcStreamBar;
};
	

};

#endif















