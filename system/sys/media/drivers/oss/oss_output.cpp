/*  OSS output plugin
 *  Copyright (C) 2003 - 2004 Arno Klenke
 *  Copyright (C) 2003 Kristian Van Der Vliet
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

#include <media/output.h>
//#include <media/server.h>
#include <media/addon.h>
#include <atheos/soundcard.h>
#include <atheos/threads.h>
#include <atheos/semaphore.h>
#include <atheos/time.h>
#include <atheos/kernel.h>
#include <atheos/threads.h>
#include <atheos/msgport.h>
#include <util/message.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <cassert>
#include "mixerview.h"
#include "resampler.h"

class OSSOutput : public os::MediaOutput
{
public:
	OSSOutput( os::String zDSPPath );
	~OSSOutput();
	os::String		GetMixerPath();
	os::String		GetIdentifier();
	uint32			GetPhysicalType()
	{
		return( os::MEDIA_PHY_SOUNDCARD );
	}
	os::View*		GetConfigurationView();
	
	bool			FileNameRequired();
	status_t 		Open( os::String zFileName );
	void 			Close();
	
	void			Clear();
	
	uint32			GetOutputFormatCount();
	os::MediaFormat_s	GetOutputFormat( uint32 nIndex );
	uint32			GetSupportedStreamCount();
	status_t		AddStream( os::String zName, os::MediaFormat_s sFormat );
	
	status_t		WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket );
	uint64			GetDelay( bool bNonSharedOnly );
	uint64			GetBufferSize( bool bNonSharedOnly );

private:

	os::String		m_zDSPPath;
	uint64			m_nFactor;
	int				m_hOSS;
	int				m_nBufferSize;
	os::MediaFormat_s m_sSrcFormat;
	os::MediaFormat_s m_sDstFormat;
	bool			m_bResample;
	os::Messenger	m_cMediaServerLink;
};



OSSOutput::OSSOutput( os::String zDSPPath )
{
	/* Set default values */
	m_hOSS = -1;
	m_zDSPPath = zDSPPath;
}

OSSOutput::~OSSOutput()
{
	Close();
}


os::String OSSOutput::GetMixerPath()
{
	if( m_zDSPPath.substr( 0, strlen( "/dev/audio" ) ) == "/dev/audio" )
	{
		os::String zMixerPath = "/dev/audio/mixer";
		zMixerPath += m_zDSPPath.substr( strlen( "/dev/audio" ), m_zDSPPath.Length() - strlen( "/dev/audio" ) );
		return( zMixerPath );
	}
	
	/* Remove "dsp/x" from the path and add "mixer/x" */
	char zPath[255];
	memset( zPath, 0, 255 );
	strncpy( zPath, m_zDSPPath.c_str(), m_zDSPPath.CountChars() - 5 );
	strcat( zPath, "mixer/" );
	char zTemp[2];
	zTemp[1] = 0;
	zTemp[0] = m_zDSPPath[m_zDSPPath.CountChars()-1];
	strcat( zPath, zTemp );
	return( os::String( zPath ) );
}

os::String OSSOutput::GetIdentifier()
{
	mixer_info sInfo;
	os::String zName = "Unknown";
	/* Try to read the mixer name */
	int nMixerDev = open( GetMixerPath().c_str(), O_RDWR );
	if( ioctl( nMixerDev, SOUND_MIXER_INFO, &sInfo ) == 0 )
		zName = os::String( sInfo.name );
	close( nMixerDev );
	return( os::String( "OSS (") + zName + os::String( ")" ) );
}

os::View* OSSOutput::GetConfigurationView()
{	
	os::MixerView* pcView = new os::MixerView( GetMixerPath().c_str(), os::Rect( 0, 0, 1, 1 ) );
	return( pcView );
}

bool OSSOutput::FileNameRequired()
{
	return( false );
}



status_t OSSOutput::Open( os::String zFileName )
{
	/* Open soundcard */
	Close();
	m_hOSS = open( m_zDSPPath.c_str(), O_WRONLY );
	if( m_hOSS < 0 )
		return( -1 );
	
	

	
	return( 0 );
}

void OSSOutput::Close()
{
	Clear();
	if( m_hOSS > -1 )
		close( m_hOSS );
	m_hOSS = -1;
	//cout<<"OSS closed"<<endl;
}


void OSSOutput::Clear()
{
	/* Stop playback */
	if( m_hOSS ) {
		ioctl( m_hOSS, SNDCTL_DSP_RESET, NULL );
		int nVal = m_sSrcFormat.nSampleRate;
		ioctl( m_hOSS, SNDCTL_DSP_SPEED, &nVal );
		m_sDstFormat.nSampleRate = nVal;
		nVal = m_sSrcFormat.nChannels;
		if( m_sSrcFormat.nChannels > 2 )
			nVal = 1;
		else
			nVal = m_sSrcFormat.nChannels - 1;
		ioctl( m_hOSS, SNDCTL_DSP_STEREO, &nVal );
		m_sDstFormat.nChannels = nVal + 1;
		nVal = AFMT_S16_LE;
		ioctl( m_hOSS, SNDCTL_DSP_SETFMT, &nVal );
	}
}

uint32 OSSOutput::GetOutputFormatCount()
{
	return( 1 );
}

os::MediaFormat_s OSSOutput::GetOutputFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	MEDIA_CLEAR_FORMAT( sFormat );
	sFormat.zName = "Raw Audio";
	sFormat.nType = os::MEDIA_TYPE_AUDIO;
	
	return( sFormat );
}

uint32 OSSOutput::GetSupportedStreamCount()
{
	return( 1 );
}

status_t OSSOutput::AddStream( os::String zName, os::MediaFormat_s sFormat )
{
	if( sFormat.zName == "Raw Audio" && sFormat.nType == os::MEDIA_TYPE_AUDIO ) {}
	else
		return( -1 );
	
	/* Set Media format */
	m_sSrcFormat = sFormat;
	int nVal = m_sSrcFormat.nSampleRate;
	ioctl( m_hOSS, SNDCTL_DSP_SPEED, &nVal );
	m_sDstFormat.nSampleRate = nVal;
	std::cout<<"SampleRate: "<<m_sSrcFormat.nSampleRate<<" -> "<<m_sDstFormat.nSampleRate<<std::endl;
	nVal = AFMT_S16_LE;
	ioctl( m_hOSS, SNDCTL_DSP_SETFMT, &nVal );
	
	if( sFormat.nChannels > 2 ) {
		//std::cout<<m_sSrcFormat.nChannels<<" Channels -> 2 Channels"<<std::endl;
		nVal = 1;
	} else {
		nVal = m_sSrcFormat.nChannels - 1;
	}
	ioctl( m_hOSS, SNDCTL_DSP_STEREO, &nVal );
	m_sDstFormat.nChannels = nVal + 1;
	
	/* Calculate size -> time factor */
	m_nFactor = (uint64)m_sDstFormat.nSampleRate * (uint64)m_sDstFormat.nChannels * 2;
	
	if( m_sDstFormat.nChannels != m_sSrcFormat.nChannels || m_sDstFormat.nSampleRate != m_sSrcFormat.nSampleRate )
		m_bResample = true;
	else
		m_bResample = false;
	
	std::cout<<"Channels: "<<m_sSrcFormat.nChannels<<" -> "<<m_sDstFormat.nChannels<<std::endl;
	
	
	/* Get buffer size */
	audio_buf_info sInfo;
	ioctl( m_hOSS, SNDCTL_DSP_GETOSPACE, &sInfo );
	m_nBufferSize = sInfo.bytes;

	return( 0 );
}



status_t OSSOutput::WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket )
{
	/* Create a new media packet and queue it */
	if( psPacket->nSize[0] < 1 )
		return( -1 );
		
	uint8* pBuffer = psPacket->pBuffer[0];
	int nSize = psPacket->nSize[0];
		

	if( m_bResample ) {
		pBuffer = ( uint8* )malloc( psPacket->nSize[0] * m_sDstFormat.nChannels / m_sSrcFormat.nChannels
													* m_sDstFormat.nSampleRate / m_sSrcFormat.nSampleRate + 4096 );
		nSize = Resample( m_sSrcFormat, m_sDstFormat, (uint16*)pBuffer, 0, INT_MAX, (uint16*)psPacket->pBuffer[0], psPacket->nSize[0] );
	}
	
	
	write( m_hOSS, pBuffer, nSize );
	return( 0 );	
}

uint64 OSSOutput::GetDelay( bool bNonSharedOnly )
{
	int nDataSize = 0;
	int nDelay;
	uint64 nTime;

	/* Add the data in the soundcard buffer */
	ioctl( m_hOSS, SNDCTL_DSP_GETODELAY, &nDelay );
	
	nDataSize += nDelay;
	nTime = nDataSize;
	nTime *= 1000;
	nTime /= (uint64)m_nFactor;
	return( nTime );
}

uint64 OSSOutput::GetBufferSize( bool bNonSharedOnly )
{
	int nDataSize = 0;
	
	uint64 nTime;

	nDataSize = m_nBufferSize;
	nTime = nDataSize;
	nTime *= 1000;
	nTime /= (uint64)m_nFactor;
	return( nTime );
}


class OSSAddon : public os::MediaAddon
{
public:
	OSSAddon( os::String zDevice )
	{
		m_bSingleMode = false;
		if( zDevice == "" )
			return;
		m_bSingleMode = true;
		printf( "OSS: %s\n", zDevice.c_str() );
		m_zDSP.push_back( zDevice );
	}
	status_t Initialize()
	{
		int nDspCount = 0;
		DIR *hAudioDir, *hDspDir;
		struct dirent *hAudioDev, *hDspNode;
		char *zCurrentPath = NULL;
		const char* pzPath = "/dev/sound/";
		
		if( m_bSingleMode )
			return( 0 );
			
		printf( "OSS device scan...\n" );

		hAudioDir = opendir( pzPath );
		if( hAudioDir == NULL )
		{
			std::cout<<"OSS: Unable to open"<<pzPath<<std::endl;
			return( -1 );
		}

		while( (hAudioDev = readdir( hAudioDir )) )
		{
			if( !strcmp( hAudioDev->d_name, "." ) || !strcmp( hAudioDev->d_name, ".." ) )
				continue;

			std::cout<<"OSS: Found audio device "<<hAudioDev->d_name<<std::endl;
			zCurrentPath = (char*)calloc( 1, strlen( pzPath ) + strlen( hAudioDev->d_name ) + 5 );
			if( zCurrentPath == NULL )
			{
				std::cout<<"OSS: Out of memory"<<std::endl;
				closedir( hAudioDir );
				return( -1 );
			}

			zCurrentPath = strcpy( zCurrentPath, pzPath );
			zCurrentPath = strcat( zCurrentPath, hAudioDev->d_name );
			zCurrentPath = strcat( zCurrentPath, "/dsp" );

			/* Look for DSP device nodes for this device */
			hDspDir = opendir( zCurrentPath );
			if( hDspDir == NULL )
			{
				std::cout<<"OSS: Unable to open"<<zCurrentPath<<std::endl;
				free( zCurrentPath );
				continue;
			}

			while( (hDspNode = readdir( hDspDir )) )
			{
				char *zDspPath = NULL;

				if( !strcmp( hDspNode->d_name, "." ) || !strcmp( hDspNode->d_name, ".." ) )
					continue;

				/* We have found a DSP device node */
				++nDspCount;

				zDspPath = (char*)calloc( 1, strlen( zCurrentPath ) + strlen( hDspNode->d_name ) + 1 );
				if( zDspPath == NULL )
				{
					printf( "OSS: Out of memory\n" );
					closedir( hDspDir );
					free( zCurrentPath );
					closedir( hAudioDir );
					return( -1 );
				}
	
				zDspPath = strcpy( zDspPath, zCurrentPath );
				zDspPath = strcat( zDspPath, "/" );
				zDspPath = strcat( zDspPath, hDspNode->d_name );

				
				std::cout<<"OSS: Added "<<zDspPath<<std::endl;
				m_zDSP.push_back( os::String( zDspPath ) );
				free( zDspPath );
			}

			closedir( hDspDir );
			free( zCurrentPath );
		}

		closedir( hAudioDir );
		return( 0 );
	}
	os::String GetIdentifier() { return( "OSS Audio" ); }
	uint32			GetOutputCount() { return( m_zDSP.size() ); }
	os::MediaOutput*	GetOutput( uint32 nIndex ) { return( new OSSOutput( m_zDSP[nIndex] ) ); }
private:
	bool m_bSingleMode;
	std::vector<os::String> m_zDSP;
};

extern "C"
{
	os::MediaAddon* init_media_addon( os::String zDevice )
	{
		return( new OSSAddon( zDevice ) );
	}

}


