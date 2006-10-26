/*  Syllable Audio Interface (HDA Codec)
 *  Copyright (C) 2006 Arno Klenke
 *
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


#include <gui/layoutview.h>
#include <gui/separator.h>
#include <atheos/kernel.h>
#include <util/settings.h>
#include <media/server.h>
#include <media/format.h>
#include <media/manager.h>
#include <media/addon.h>
#include <media/input.h>
#include <media/output.h>
#include <storage/file.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cassert>
#include "resampler.h"
#include "hda.h"
#include "hda_channel.h"


using namespace os;


/* Useful macro to create MediaFormat_s structures */
#define CREATE_FORMAT( channels, samplerate ) \
{ \
sFormat.nType = os::MEDIA_TYPE_AUDIO; \
sFormat.zName = "Raw Audio"; \
sFormat.nSampleRate = samplerate; \
sFormat.nChannels = channels; \
m_asFormats.push_back( sFormat ); \
}

HDAAudioOutputStream::HDAAudioOutputStream( HDAAudioDriver* pcDriver, os::String zDevice, bool bSPDIF )
{
	m_zDevice = zDevice;
	m_nFd = -1;
	m_pcDriver = pcDriver;
	m_bSPDIF = bSPDIF;
	
	/* Create formats (preferred ones come first) */
	os::MediaFormat_s sFormat;
	m_asFormats.clear();
	MEDIA_CLEAR_FORMAT( sFormat );
	
	int nMaxChannels = pcDriver->GetCardMaxChannels();
	if( pcDriver->GetCardFormats() & ( 1 << 5 ) )
	{
		/* 44.1khz - 2 channels */	
		CREATE_FORMAT( 2, 44100 );
		/* 44.1khz - 4 channels */
		if( nMaxChannels >= 4 )
			CREATE_FORMAT( 4, 44100 );
		/* 44.1khz - 6 channels */
		if( nMaxChannels >= 6 )
			CREATE_FORMAT( 6, 44100 );
	}
	
	/* 48khz - 2 channels */	
	CREATE_FORMAT( 2, 48000 );
	/* 48khz - 4 channels */
	if( nMaxChannels >= 4 )
		CREATE_FORMAT( 4, 48000 );
	/* 48khz - 6 channels */
	if( nMaxChannels >= 6 )
		CREATE_FORMAT( 6, 48000 );
	/* Everything else supported with the help of the resampler */
	CREATE_FORMAT( 0, 0 );
}

HDAAudioOutputStream::~HDAAudioOutputStream()
{
	if( m_nFd )
		close( m_nFd );
}

uint32 HDAAudioOutputStream::GetPhysicalType()
{
	if( m_bSPDIF )
		return( MEDIA_PHY_SOUNDCARD_SPDIF_OUT );
	else
		return( MEDIA_PHY_SOUNDCARD_LINE_OUT );
}

bool HDAAudioOutputStream::FileNameRequired()
{
	return( false );
}

uint64 HDAAudioOutputStream::ConvertToTime( uint32 nBytes )
{
	uint64 nTime = nBytes;
	nTime *= 1000;
	nTime /= (uint64)m_nFactor;
	return( nTime );
}


/** Returns the delay.
 * \par Description:
 * This method is called by the application to get the current delay in msecs.
 * The implementation of this class will calculate this value using the
 * GetCurrentPosition() method.
 * \author	Arno Klenke
 *****************************************************************************/
uint64 HDAAudioOutputStream::GetDelay( bool bNonSharedOnly )
{
	uint32 nValue;
	if( m_nFd < 0 )
		return( 0 );
	ioctl( m_nFd, HDA_GET_DELAY, &nValue );
	uint64 nVal = ConvertToTime( nValue );
	return( nVal );
}

/** Returns the buffer size.
 * \par Description:
 * This method is called by the application to get the buffer size in msecs.
 * The implementation of this class will calculate this value.
 * \author	Arno Klenke
 *****************************************************************************/
uint64 HDAAudioOutputStream::GetBufferSize( bool bNonSharedOnly )
{
	uint32 nValue;
	if( m_nFd < 0 )
		return( 0 );
	ioctl( m_nFd, HDA_GET_BUFFER_SIZE, &nValue );
	return( ConvertToTime( nValue ) );
}


os::String HDAAudioOutputStream::GetIdentifier()
{
	return( m_pcDriver->GetIdentifier() );
}

os::View* HDAAudioOutputStream::GetConfigurationView()
{
	return( m_pcDriver->GetConfigurationView() );
}


uint32 HDAAudioOutputStream::GetOutputFormatCount()
{
	return( m_asFormats.size() );
}

os::MediaFormat_s HDAAudioOutputStream::GetOutputFormat( uint32 nIndex )
{
	return( m_asFormats[nIndex] );
}

status_t HDAAudioOutputStream::Open( os::String zFileName )
{
	m_nFd = open( m_zDevice.c_str(), O_RDWR );
	if( m_nFd < 0 )
		return( m_nFd );
	printf( "Device opened\n" );
	return( 0 );
}

uint32 HDAAudioOutputStream::GetSupportedStreamCount()
{
	return( 1 );
}

status_t HDAAudioOutputStream::AddStream( os::String zName, os::MediaFormat_s sFormat )
{	
	HDAAudioDriver* pcDriver = m_pcDriver;
	bool bResample = false;
	m_sSrcFormat = m_sDstFormat = sFormat;
	
	
	/* Set hardware support */
	os::MediaFormat_s sHWFormat = sFormat;
	
	/* Check hardware limitations */
	if( sFormat.nChannels > pcDriver->GetCardMaxChannels() )
	{
		printf( "Card does only support %i channels\n", pcDriver->GetCardMaxChannels( ) );
		sHWFormat.nChannels = 2;
		bResample = true;
	}
	if( sFormat.nSampleRate > pcDriver->GetCardMaxSampleRate() )
	{
		printf( "Card does only supports %iHz\n", pcDriver->GetCardMaxSampleRate() );
		sHWFormat.nSampleRate = pcDriver->GetCardMaxSampleRate();
		bResample = true;
	}
	
	
	/* Check if we support the rate */
	if( sHWFormat.nSampleRate != 44100 && sHWFormat.nSampleRate != 48000 )
	{
		sHWFormat.nSampleRate = 48000;
		bResample = true;
	}
	if( sHWFormat.nSampleRate == 44100 && !( pcDriver->GetCardFormats() & ( 1 << 5 ) ) )
	{
		sHWFormat.nSampleRate = 48000;
		bResample = true;
	}
	
	
	HDAFormat_s sHDAFormat;
	sHDAFormat.nChannels = sHWFormat.nChannels;
	sHDAFormat.nSampleRate = sHWFormat.nSampleRate;
	sHDAFormat.nResolution = 16;
	
	if( ioctl( m_nFd, HDA_SET_FORMAT, &sHDAFormat ) != 0 )
	{
		printf( "Card did not accept audio format. Fall back to defaults.\n" );
		sHWFormat.nChannels = 2;
		sHWFormat.nSampleRate = 48000;
		sHDAFormat.nChannels = sHWFormat.nChannels;
		sHDAFormat.nSampleRate = sHWFormat.nSampleRate;
		bResample = true;
		if( ioctl( m_nFd, HDA_SET_FORMAT, &sHDAFormat ) != 0 )
		{
			printf( "Card does not accept default hda format!\n" );
			return( -EINVAL );
		}
	}
	
	
	/* Enable resampler */
	if( bResample )
	{
		m_bResample = true;
		m_sDstFormat = sHWFormat;
		printf("Resampler enabled %i -> %i!\n", (int)sFormat.nSampleRate, (int)sHWFormat.nSampleRate );
	} else
		m_bResample = false;
	
	/* Calculate size -> time factor */
	m_nFactor = (uint64)sHWFormat.nSampleRate * (uint64)sHWFormat.nChannels * 2;
	return( 0 );
}

status_t HDAAudioOutputStream::WritePacket( uint32 nIndex, os::MediaPacket_s* psPacket )
{
	/* Check that we have at least one free fragment */
	uint8* pBuffer = (uint8*)psPacket->pBuffer[0];
	ssize_t nSize = psPacket->nSize[0];
	
	
	if( m_bResample )
	{
		
		pBuffer = ( uint8* )malloc( psPacket->nSize[0] * m_sDstFormat.nChannels / m_sSrcFormat.nChannels
													* m_sDstFormat.nSampleRate / m_sSrcFormat.nSampleRate + 4096 );
		nSize = Resample( m_sSrcFormat, m_sDstFormat, (uint16*)pBuffer, 0, INT_MAX, (uint16*)psPacket->pBuffer[0], psPacket->nSize[0] );
		//printf("Resample! %i %i\n", (int)psPacket->nSize[0], (int)nSize );
	}
	
	
	write( m_nFd, pBuffer, nSize );

	if( m_bResample )
		free( pBuffer );
	
	return( 0 );
}

void HDAAudioOutputStream::Clear()
{
	ioctl( m_nFd, HDA_CLEAR, NULL );
}

void HDAAudioOutputStream::Close()
{
	if( m_nFd )
		close( m_nFd );
	m_nFd = -1;
}

//-----------------------------------------------------------------------
#if 0
HDAAudioInputStream::HDAAudioInputStream( HDAAudioDriver* pcDriver, os::String zDevice )
{
	m_zDevice = zDevice;
	m_nFd = -1;
	m_pcDriver = pcDriver;
}

HDAAudioInputStream::~HDAAudioInputStream()
{
	if( m_nFd )
		close( m_nFd );
}

uint32 HDAAudioInputStream::GetPhysicalType()
{
	return( MEDIA_PHY_SOUNDCARD_MIC_IN );
}

bool HDAAudioInputStream::FileNameRequired()
{
	return( false );
}


bool HDAAudioInputStream::PacketBased()
{
	return( true );
}

bool HDAAudioInputStream::StreamBased()
{
	return( true );
}

status_t HDAAudioInputStream::Open( os::String zFileName )
{
	m_nFd = open( m_zDevice.c_str(), O_RDWR );
	if( m_nFd < 0 )
		return( m_nFd );
	printf( "Device opened\n" );
	/* FIXME */
	HDAFormat_s sHDAFormat;
	sHDAFormat.nChannels = 2;
	sHDAFormat.nSampleRate = 48000;
	sHDAFormat.nResolution = 16;
	
	
	if( ioctl( m_nFd, HDA_SET_FORMAT, &sHDAFormat ) != 0 )
	{
		printf( "Card does not accept default ac97 format!\n" );
		return( -EINVAL );
	}
	
	return( 0 );
}


void HDAAudioInputStream::Clear()
{
	ioctl( m_nFd, HDA_CLEAR, NULL );
}

void HDAAudioInputStream::Close()
{
	if( m_nFd )
		close( m_nFd );
	m_nFd = -1;
}

uint32 HDAAudioInputStream::GetTrackCount()
{
	return( 1 );
}

os::String HDAAudioInputStream::GetIdentifier()
{
	return( m_pcDriver->GetIdentifier() );
}

os::View* HDAAudioInputStream::GetConfigurationView()
{
	return( m_pcDriver->GetConfigurationView() );
}


uint32 HDAAudioInputStream::GetStreamCount()
{
	return( 1 );
}

MediaFormat_s HDAAudioInputStream::GetStreamFormat( uint32 nIndex )
{
	/* Fixme: Offer more formats */
	os::MediaFormat_s sFormat;
	
	sFormat.nType = os::MEDIA_TYPE_AUDIO;
	sFormat.zName = "Raw Audio";
	sFormat.nBitRate = 0;
	sFormat.nSampleRate = 48000;
	sFormat.nChannels = 2;
	sFormat.bVBR = false;

	return( sFormat );
}

status_t HDAAudioInputStream::ReadPacket( MediaPacket_s* psPacket )
{
	// Fixme: Use fragment size
	psPacket->pBuffer[0] = (uint8*)malloc( PAGE_SIZE );
	psPacket->nStream = 0;
	psPacket->nTimeStamp = ~0;
	psPacket->nSize[0] = read( m_nFd, psPacket->pBuffer[0], PAGE_SIZE );
	return( 0 );
}


void HDAAudioInputStream::FreePacket( MediaPacket_s* psPacket )
{
	free( psPacket->pBuffer[0] );
}
#endif
//-----------------------------------------------------------------------

HDAAudioDriver::HDAAudioDriver( os::String zDevice )
{
	m_zDevice = zDevice;
}

HDAAudioDriver::~HDAAudioDriver()
{
}

status_t HDAAudioDriver::Initialize()
{
	/* Open device */
	int nFd = open( m_zDevice.c_str(), O_RDONLY );
	if( nFd < 0 )
		return( nFd );
	
	/* Get card info */
	
	HDACardInfo_s sCardInfo;
	if( ioctl( nFd, HDA_GET_CARD_INFO, &sCardInfo ) != 0 )
		return( -EIO );
	
	printf( "HDA: %s\n", sCardInfo.zName );
	m_zName = sCardInfo.zName;
	m_bRecord = sCardInfo.bRecord;

	m_bSPDIF = sCardInfo.bSPDIF;
	m_nDevHandle = sCardInfo.nDeviceHandle;
	m_nCardMaxChannels = sCardInfo.nMaxChannels;
	m_nCardMaxSampleRate = sCardInfo.nMaxSampleRate;
	m_nCardFormats = sCardInfo.nFormats;
	
	/* Read nodes */
	HDANode_s sNode;
	int i = 0;
	while( 1 )
	{
		sNode.nNode = i;
		if( ioctl( nFd, HDA_GET_NODE, &sNode ) != 0 )
		{
			break;
		}
		m_asNodes.push_back( sNode );
		i++;
	}
	
	/* Read output paths */
	HDAOutputPath_s sPath;
	i = 0;
	while( 1 )
	{
		sPath.nDacNode = i;
		if( ioctl( nFd, HDA_GET_OUTPUT_PATH, &sPath ) != 0 )
		{
			break;
		}
		m_asOutputPaths.push_back( sPath );
		i++;
	}
	
	
	#if 0	
	/* Get HDA info */
	HDAAudioDriver_s sInfo;
	if( ioctl( nFd, HDA_GET_CODEC_INFO, &sInfo ) != 0 )
		return( -EIO );
		
	m_zID = sInfo.zID;
	m_nCodecs = sInfo.nCodecs;
	memcpy( m_nID, sInfo.nID, 4 * sizeof( uint32 ) );
	memcpy( m_nBasicID, sInfo.nBasicID, 4 * sizeof( uint16 ) );
	memcpy( m_nExtID, sInfo.nExtID, 4 * sizeof( uint16 ) );
	memcpy( m_nChannels, sInfo.nChannels, 4 * sizeof( int ) );
	m_nMaxChannels = sInfo.nMaxChannels;
	
	
	/* Copy mixer information */
	for( int i = 0; i < sInfo.nNumMixer; i++ )
	{
		HDAMixer_s* psMixer = &sInfo.asMixer[i];
		Mixer sMixer( psMixer->bStereo, psMixer->bInvert, "Mixer", psMixer->nCodec, psMixer->nReg, psMixer->nScale );
		sMixer.m_nValue = HDARead( nFd, psMixer->nCodec, psMixer->nReg );
		if( psMixer->nScale != 0 )
			m_cMixers.push_back( sMixer );
	}
	#endif
	/* Close device */
	close( nFd );
	
	return( 0 );
}


os::String HDAAudioDriver::GetIdentifier()
{
	return( m_zName );
}

uint32 HDAAudioDriver::GetOutputCount()
{
	if( m_bRecord )
		return( 0 );
	return( 1 );
}

os::MediaOutput* HDAAudioDriver::GetOutput( uint32 nIndex )
{
	if( m_bRecord )
		return( NULL );
	return( new HDAAudioOutputStream( this, m_zDevice, m_bSPDIF ) );
}

uint32 HDAAudioDriver::GetInputCount()
{
	if( !m_bRecord )
		return( 0 );
	return( 1 );
}

os::MediaInput* HDAAudioDriver::GetInput( uint32 nIndex )
{
	if( !m_bRecord )
		return( NULL );
//	return( new HDAAudioInputStream( this, m_zDevice ) );
}
#if 0
/** Load mixer settings.
 * \par Description:
 * Load mixer settings.
 * \internal
 * \author	Arno Klenke
 *****************************************************************************/
status_t HDAAudioDriver::HDALoadMixerSettings()
{
	/* Write mixer settings */
	os::Path cPath = os::String( "/system/config" );
	cPath.Append( os::String( "HDAMixer_" ) + m_zID );
	
	os::File* pcFile = new os::File();
	if( pcFile->SetTo( cPath, O_RDWR ) != 0 )
	{
		delete pcFile;
		return( -1 );
	}
	os::Settings cSettings( pcFile );
	if( cSettings.Load() != 0 )
	{
		printf("Failed to load settings!\n");
		return( -1 );
	}
	int16 nCodec;
	int16 nReg; 
	int16 nValue;
	
	int nIndex = 0;
	while( cSettings.FindInt16( "codec", &nCodec, nIndex ) == 0 )
	{
		cSettings.FindInt16( "reg", &nReg, nIndex );
		cSettings.FindInt16( "value", &nValue, nIndex );
		HDAWriteMixer( nCodec, nReg, nValue );
		nIndex++;
	}
	return( 0 );
}

/** Savs mixer settings.
 * \par Description:
 * Save mixer settings.
 * \internal
 * \author	Arno Klenke
 *****************************************************************************/
void HDAAudioDriver::HDASaveMixerSettings()
{
	/* Write mixer settings */
	os::Path cPath = os::String( "/system/config" );
	cPath.Append( os::String( "HDAMixer_" ) + m_zID );
	
	os::File* pcFile = new os::File();
	if( pcFile->SetTo( cPath, O_TRUNC | O_CREAT | O_RDWR ) != 0 )
	{
		return;
	}
	
	os::Settings cSettings( pcFile );
	for( uint i = 0; i < m_cMixers.size(); i++ )
	{
		cSettings.AddInt16( "codec", m_cMixers[i].m_nCodec );
		cSettings.AddInt16( "reg", m_cMixers[i].m_nReg );
		cSettings.AddInt16( "value", m_cMixers[i].m_nValue );
		//printf( "Save %i %x\n", m_cMixers[i].m_nCodec, (uint)m_cMixers[i].m_nValue );
	}
	cSettings.Save();
	pcFile->Flush();
}

/** Write one mixer value.
 * \par Description:
 * Write one mixer value.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \param nValue - Volume value;
 * \internal
 * \author	Arno Klenke
 *****************************************************************************/

void HDAAudioDriver::HDAWriteMixer( int nFd, int nCodec, uint8 nReg, uint16 nValue )
{
	for( uint i = 0; i < m_cMixers.size(); i++ )
	{
		if( ( m_cMixers[i].m_nCodec == nCodec ) && ( m_cMixers[i].m_nReg == nReg ) )
		{
			m_cMixers[i].m_nValue = nValue;
			HDAWrite( nFd, nCodec, nReg, nValue );
			break;
		}
	}
}

/** Writes a volume to a stereo mixer.
 * \par Description:
 * Writes a volume to a stereo mixer.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \param nLeft - Left volume. Value between 0 and 100.
 * \param nRight - Right volume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
void HDAAudioDriver::HDAWriteStereoMixer( int nFd, int nCodec, uint8 nReg, int nLeft, int nRight )
{
	/* Find mixer */
	int nScale = 32;
	Mixer* pcMixer = NULL;
	for( uint i = 0; i < m_cMixers.size(); i++ )
	{
		if( ( m_cMixers[i].m_nCodec == nCodec ) && ( m_cMixers[i].m_nReg == nReg ) )
		{
			nScale = ( 1 << m_cMixers[i].m_nScale );
			pcMixer = &m_cMixers[i];
			break;
		}
	}
	
	if( pcMixer == NULL ) {
		printf("Error: Could not find mixer %i:%x\n", nCodec, (uint)nReg );
		return;
	}
	
	if( nLeft == 0 && nRight == 0 )
		pcMixer->m_nValue = HDA_MUTE;
	else
	{
		
		/* Calculate value */
		if( pcMixer->m_bInvert ) {
			nRight = ( ( 100 - nRight ) * nScale ) / 100;
			nLeft = ( ( 100 - nLeft ) * nScale ) / 100;
		} else {
			nRight = ( ( nRight ) * nScale ) / 100;
			nLeft = ( ( nLeft ) * nScale ) / 100;
		}
		if( nRight >= nScale )
			nRight = nScale - 1;
		if( nLeft >= nScale )
			nLeft = nScale - 1;
		pcMixer->m_nValue = ( nLeft << 8 ) | nRight;
	}
	//printf( "Write mixer %x::%x\n", (uint)nReg, (uint)pcMixer->m_nValue );
	HDAWrite( nFd, nCodec, nReg, pcMixer->m_nValue );
}

/** Reads a volume from a stereo mixer.
 * \par Description:
 * Reads a volume from a stereo mixer.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \param pnLeft - Will contain the left volume. Value between 0 and 100.
 * \param pnRight - Will contain the right volume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
void HDAAudioDriver::HDAReadStereoMixer( int nFd, int nCodec, uint8 nReg, int *pnLeft, int *pnRight )
{
	
	/* Find mixer */
	int nScale = 32;
	Mixer* pcMixer = NULL;
	for( uint i = 0; i < m_cMixers.size(); i++ )
	{
		if( ( m_cMixers[i].m_nCodec == nCodec ) && ( m_cMixers[i].m_nReg == nReg ) )
		{
			nScale = ( 1 << m_cMixers[i].m_nScale );
			pcMixer = &m_cMixers[i];
			break;
		}
	}
	
	if( pcMixer == NULL ) {
		printf("Error: Could not find mixer %i:%x\n", nCodec, (uint)nReg );
		return;
	}
	
	if( pcMixer->m_nValue == HDA_MUTE )
	{
		*pnLeft = *pnRight = 0;
		return;
	}
	
	if( pcMixer->m_bInvert ) {
		*pnLeft = 100 - ( ( (pcMixer->m_nValue >> 8 ) * 100 ) / nScale );
		*pnRight = 100 - ( ( ( ( pcMixer->m_nValue & 0xff ) & 0xff ) * 100 ) / nScale );
	} else {
		*pnLeft = ( ( (pcMixer->m_nValue >> 8 ) * 100 ) / nScale );
		*pnRight = ( ( ( ( pcMixer->m_nValue & 0xff ) & 0xff ) * 100 ) / nScale );
	}
	
}

/** Writes a volume to a mono mixer.
 * \par Description:
 * Writes a volume to a mono mixer.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \param nVolume - Volume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
void HDAAudioDriver::HDAWriteMonoMixer( int nFd, int nCodec, uint8 nReg, int nVolume )
{
	/* Find mixer */
	int nScale = 32;
	Mixer* pcMixer = NULL;
	for( uint i = 0; i < m_cMixers.size(); i++ )
	{
		if( ( m_cMixers[i].m_nCodec == nCodec ) && ( m_cMixers[i].m_nReg == nReg ) )
		{
			nScale = ( 1 << m_cMixers[i].m_nScale );
			pcMixer = &m_cMixers[i];
			break;
		}
	}
	
	if( pcMixer == NULL ) {
		printf("Error: Could not find mixer %i:%x\n", nCodec, (uint)nReg );
		return;
	}
	
	if( nVolume == 0 )
		pcMixer->m_nValue = HDA_MUTE;
	else
	{
		
		/* Calculate value */
		if( pcMixer->m_bInvert ) {
			nVolume = ( ( 100 - nVolume ) * nScale ) / 100;
		} else {
			nVolume = ( ( nVolume ) * nScale ) / 100;
		}
		if( nVolume >= nScale )
			nVolume = nScale - 1;
		pcMixer->m_nValue = nVolume;
	}
	
	HDAWrite( nFd, nCodec, nReg, pcMixer->m_nValue );
	printf( "Write mixer %x::%x\n", (uint)nReg, (uint)pcMixer->m_nValue );
}

/** Reads a volume from a mono mixer.
 * \par Description:
 * Reads a volume from a mono mixer.
 * \param nCodec - Codec.
 * \param nReg - Register.
 * \param pnVolume - Will contain the olume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
void HDAAudioDriver::HDAReadMonoMixer( int nFd, int nCodec, uint8 nReg, int *pnVolume )
{
	
	/* Find mixer */
	int nScale = 32;
	Mixer* pcMixer = NULL;
	for( uint i = 0; i < m_cMixers.size(); i++ )
	{
		if( ( m_cMixers[i].m_nCodec == nCodec ) && ( m_cMixers[i].m_nReg == nReg ) )
		{
			nScale = ( 1 << m_cMixers[i].m_nScale );
			pcMixer = &m_cMixers[i];
			break;
		}
	}
	
	if( pcMixer == NULL ) {
		printf("Error: Could not find mixer %i:%x\n", nCodec, (uint)nReg );
		return;
	}
	
	if( pcMixer->m_nValue == HDA_MUTE )
	{
		*pnVolume = 0;
		return;
	}
	
	
	if( pcMixer->m_bInvert ) {
		*pnVolume = 100 - ( ( (pcMixer->m_nValue ) * 100 ) / nScale );
	} else {
		*pnVolume = ( ( (pcMixer->m_nValue ) * 100 ) / nScale );
	}
	
}



/** Writes a volume to the microphone mixer.
 * \par Description:
 * Writes a volume to the microphone mixer.
 * \param nCodec - Codec.
 * \param nValue - Volume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
void HDAAudioDriver::HDAWriteMicMixer( int nFd, int nCodec, int nValue )
{
	/* Find mixer */
	int nScale = 32;
	Mixer* pcMixer = NULL;
	for( uint i = 0; i < m_cMixers.size(); i++ )
	{
		if( ( m_cMixers[i].m_nCodec == nCodec ) && ( m_cMixers[i].m_nReg == HDA_MIC_VOL ) )
		{
			nScale = ( 1 << m_cMixers[i].m_nScale );
			pcMixer = &m_cMixers[i];
			break;
		}
	}
	
	if( pcMixer == NULL ) {
		printf("Error: Could not find mixer %i:%x\n", nCodec, HDA_MIC_VOL );
		return;
	}
	
	if( nValue == 0 )
		pcMixer->m_nValue = HDA_MUTE;
	else
	{
		/* Calculate value */
		pcMixer->m_nValue = pcMixer->m_nValue & ~0x801f;
		nValue = ( ( 100 - nValue ) * nScale ) / 100;
		if( nValue >= nScale )
			nValue = nScale - 1;
		pcMixer->m_nValue |= nValue;
	}
	//printf( "Write mixer %x::%x\n", HDA_MIC_VOL, (uint)pcMixer->m_nValue );
	HDAWrite( nFd, nCodec, HDA_MIC_VOL, pcMixer->m_nValue );
}

/** Reads the microphone mixer.
 * \par Description:
 *  Reads the microphone mixer.
 * \param nCodec - Codec.
 * \param pnValue - Will contain the volume. Value between 0 and 100.
 * \author	Arno Klenke
 *****************************************************************************/
void HDAAudioDriver::HDAReadMicMixer( int nFd, int nCodec, int* pnValue )
{
	/* Find mixer */
	int nScale = 32;
	Mixer* pcMixer = NULL;
	for( uint i = 0; i < m_cMixers.size(); i++ )
	{
		if( ( m_cMixers[i].m_nCodec == nCodec ) && ( m_cMixers[i].m_nReg == HDA_MIC_VOL ) )
		{
			nScale = ( 1 << m_cMixers[i].m_nScale );
			pcMixer = &m_cMixers[i];
			break;
		}
	}
	
	if( pcMixer == NULL ) {
		printf("Error: Could not find mixer %i:%x\n", nCodec, HDA_MIC_VOL );
		return;
	}
	
	if( ( pcMixer->m_nValue & 0x801f ) == HDA_MUTE )
	{
		*pnValue = 0;
		return;
	}
	
	*pnValue = 100 - ( ( ( pcMixer->m_nValue & 0x1f ) * 100 ) / nScale );
}

/** Enables or disables the spdif output.
 * \par Description:
 * Enables or disables the spdif output.
 * \par Note:
 * Use the HDASupportsSPDIF() method first.
 * \param nCodec - Codec.
 * \param bEnable - Enable or disable.
 * \author	Arno Klenke
 *****************************************************************************/

status_t HDAAudioDriver::HDAEnableSPDIF( int nFd, int nCodec, bool bEnable )
{
	if( !HDASupportsSPDIF( nCodec ) )
		return( -EINVAL );
	
	uint16 nVal = HDARead( nFd, nCodec, HDA_EXTENDED_STATUS );
	nVal &= ~HDA_EA_SPDIF;
	if( bEnable )
		nVal |= HDA_EA_SPDIF;
	HDAWrite( nFd, nCodec, HDA_EXTENDED_STATUS, nVal );
	printf( "EXT %x\n", (uint)nVal );
	
	return( 0 );
}

#endif
 
class HDAMixerView : public os::LayoutView
{
public:
	HDAMixerView( int nFd, os::Rect cFrame, os::String zName ) : os::LayoutView( cFrame, zName )
	{
		m_nFd = nFd;
	}
	~HDAMixerView()
	{
		printf( "Closing mixer\n" );
		close( m_nFd );
	}
private:
	int m_nFd;
};

os::View* HDAAudioDriver::GetConfigurationView()
{
	os::HDAChannel* pcChannel;
	
	/* Open */
	int nFd = open( m_zDevice.c_str(), O_RDONLY );
	if( nFd < 0 )
		return( NULL );
	
	
	os::LayoutView* pcView = new HDAMixerView( nFd, os::Rect( 0, 0, 0, 0 ), "layout" );
	os::HLayoutNode* pcNode = new os::HLayoutNode( "ac97_h_layout" );
	
	/* Add mixer channels */
	pcNode->AddChild( new os::HLayoutSpacer( "spacer", 5.0f, 5.0f ) );
	for( uint i = 0; i < m_asOutputPaths.size(); i++ )
	{	
		pcChannel = new os::HDAChannel( nFd, os::Rect(), "hda_channel",
		this, m_asOutputPaths[i] );
		pcNode->AddChild( pcChannel, 0.0f );
		pcNode->AddChild( new os::HLayoutSpacer( "spacer", 5.0f, 5.0f ) );

	}
	
	pcNode->AddChild( new os::HLayoutSpacer( "" ) );
	
	pcView->SetRoot( pcNode );
	return( pcView );
}
#if 0
status_t HDAAudioDriver::HDAWait( int nFd, int nCodec )
{
	assert( nFd >= 0 );
	HDARegOp_s sOp;
	sOp.nCodec = nCodec;
	return( ioctl( nFd, HDA_WAIT, &sOp ) );
}

status_t HDAAudioDriver::HDAWrite( int nFd, int nCodec, uint8 nReg, uint16 nVal )
{
	assert( nFd >= 0 );
	HDARegOp_s sOp;
	sOp.nCodec = nCodec;
	sOp.nReg = nReg;
	sOp.nVal = nVal;
	//printf( "Ac97Write %x %x\n", nReg, nVal );
	return( ioctl( nFd, HDA_WRITE, &sOp ) );
}

uint16 HDAAudioDriver::HDARead( int nFd, int nCodec, uint8 nReg )
{
	assert( nFd >= 0 );
	HDARegOp_s sOp;
	sOp.nCodec = nCodec;
	sOp.nReg = nReg;
	ioctl( nFd, HDA_READ, &sOp );
	return( sOp.nVal );
}


#endif
extern "C"
{
	os::MediaAddon* init_media_addon( os::String zDevice )
	{
		return( new HDAAudioDriver( zDevice ) );
	}

}





























































