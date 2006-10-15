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
	
	void SetMasterVolume( int nValue );
	void StreamChanged( uint32 nNum );

	int FindMixers( const char *pzPath );

private:
	MediaServer*	m_pcServer;
	std::vector<os::MediaOutput*> m_apcOutputs;
	TabView*		m_pcTabs;
	bool			m_bStreamActive[MEDIA_MAX_AUDIO_STREAMS];
	BarView*		m_pcStreamBar;
};
	

};

#endif















