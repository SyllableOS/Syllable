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
 
#ifndef _MEDIASERVER_H_
#define _MEDIASERVER_H_
 
#include <gui/window.h>
#include <gui/tabview.h>
#include <util/application.h>
#include <util/message.h>
#include <util/messenger.h>
#include <util/looper.h>
#include <util/string.h>
#include <util/settings.h>
#include <util/resources.h>
#include <atheos/soundcard.h>
#include <atheos/msgport.h>
#include <atheos/threads.h>
#include <atheos/image.h>
#include <atheos/areas.h>
#include <atheos/time.h>
#include <media/manager.h>
#include <media/server.h>
#include <media/input.h>
#include <media/codec.h>
#include <media/output.h>
#include <media/soundplayer.h>
#include <storage/directory.h>
#include <storage/file.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <vector>

namespace os
{
	

#define MEDIA_MAX_AUDIO_STREAMS 64
#define MEDIA_MAX_STREAM_PACKETS 10
#define MEDIA_MAX_DSPS 8

struct MediaPlugin_s {
	String zFileName;
	int nID;
};

struct MediaAudioStream_s {
	bool bUsed;
	char zName[255];
	bool bActive;
	bool bResample;
	int32 nChannels;
	int32 nSampleRate;
	area_id hArea;
	uint8* pAddress;
	int nVolume;
	uint32 nValueCount;
	float vValue;
	bigtime_t nBufferPlayed;
};

struct MediaDSP_s {
	bool bUsed;
	char zName[MAXNAMLEN];
	char zPath[4096];
};

struct MediaMixer_s {
	bool bUsed;
	char zPath[4096];
	int hMixerDev;
	View* pcFrame;
};

class MediaControls;

class MediaServer : public Application
{
public:
	MediaServer();
	~MediaServer();
	
	bool OkToQuit();
	
	void FlushThread();
	
	void HandleMessage( Message* pcMessage );
	thread_id Start();
	
	MediaAudioStream_s* GetAudioStream( uint32 nNum ) 
	{
		return( &m_sAudioStream[nNum] );
	}
	void SetMasterVolume( int nVolume ) 
	{
		m_nMasterVolume = nVolume;
	}
	void ControlsHidden( Rect cFrame )
	{
		m_cControlsFrame = cFrame;
		m_bControlsShown = false;
	}
private:
	void SaveSettings();
	
	int FindDsps( const char *pzPath );

	bool OpenSoundCard( int nDevice );
	void CloseSoundCard();
	
	void GetDefaultInput( Message* pcMessage );
	void SetDefaultInput( Message* pcMessage );
	void GetDefaultAudioOutput( Message* pcMessage );
	void GetDefaultVideoOutput( Message* pcMessage );
	void SetDefaultVideoOutput( Message* pcMessage );
	void SetDefaultAudioOutput( Message* pcMessage );
	void GetStartupSound( Message* pcMessage );
	void SetStartupSound( Message* pcMessage );
	void CreateAudioStream( Message* pcMessage );
	void DeleteAudioStream( Message* pcMessage );
	
	void FlushAudioStream( Message* pcMessage );
	void GetDelay( Message* pcMessage );
	void GetPercentage( Message* pcMessage );
	void Clear( Message* pcMessage );
	void Start( Message* pcMessage );
	
	uint32 Resample( os::MediaFormat_s sFormat, uint16* pDst, uint16* pSrc, uint32 nLength );

	void GetDspCount( Message* pcMessage );
	void GetDspInfo( Message* pcMessage );
	void GetDefaultDsp( Message* pcMessage );
	void SetDefaultDsp( Message* pcMessage );
	
	String 			m_zDefaultInput;
	String 			m_zDefaultAudioOutput;
	String 			m_zDefaultVideoOutput;
	String			m_zStartupSound;
	MediaAudioStream_s m_sAudioStream[MEDIA_MAX_AUDIO_STREAMS];
	os::MediaPacket_s* m_psPacket[MEDIA_MAX_AUDIO_STREAMS][MEDIA_MAX_STREAM_PACKETS];
	uint32			m_nQueuedPackets[MEDIA_MAX_AUDIO_STREAMS];
	MediaDSP_s		m_sDsps[MEDIA_MAX_DSPS];
	int				m_nDspCount;
	int				m_nDefaultDsp;
	int				m_hOSS;
	int				m_nBufferSize;
	sem_id			m_hLock;
	sem_id			m_hClearLock;
	thread_id		m_hThread;
	bool			m_bRunThread;
	uint32			m_nActiveStreamCount;
	signed int*		m_pMixBuffer;
	uint32*			m_pValueBuffer;
	uint32 			m_nMasterValueCount;
	float 			m_vMasterValue;
	int				m_nMasterVolume;
	
	MediaControls*	m_pcControls;
	Rect			m_cControlsFrame;
	bool			m_bControlsShown;
};
	

};

#endif







































