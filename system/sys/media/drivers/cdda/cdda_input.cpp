/*  CDDA Input plugin
 *  Copyright (C) 2004 Kristian Van Der Vliet
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stropts.h>
#include <media/input.h>
#include <media/codec.h>
#include <media/addon.h>
#include <atheos/cdrom.h>

#include <iostream>
#include <vector>
#include <sys/stat.h>

#define NUM_BUFS		32
#define OVERLAP			4

#define	min(a,b)	(((a)<(b)) ? (a) : (b) )

class CddaInput : public os::MediaInput
{
public:
	CddaInput( os::String zDevicePath );
	~CddaInput();
	uint32			GetPhysicalType()
	{
		return( os::MEDIA_PHY_CD_DVD_DIGITAL_IN );
	}
	os::String 		GetIdentifier();
	os::View*		GetConfigurationView();
	
	bool			FileNameRequired();
	status_t 		Open( os::String zFileName );
	void 			Close();

	bool			PacketBased();
	bool			StreamBased();
	
	uint32			GetTrackCount();
	uint32			SelectTrack( uint32 nTrack );
	
	status_t		ReadPacket( os::MediaPacket_s* psPacket );
	void			FreePacket( os::MediaPacket_s* psPacket );
	
	uint64			GetLength();
	uint64			GetCurrentPosition();
	uint64			Seek( uint64 nPosition );
	
	uint32			GetStreamCount();
	os::MediaFormat_s	GetStreamFormat( uint32 nIndex );

private:
	void			InvalidateBlockCache();
	void			CreateBuffers();

private:
	os::String m_zDevicePath;
	int				m_hCdrom;
	cdrom_toc_s*	m_psToc;

	int				m_nSelected;	// Current selected track
	uint32			m_nCurentBlock;	// Last block read

	uint8*			m_nLastBlocks[NUM_BUFS];
	uint8*			m_nTheseBlocks[NUM_BUFS];
	uint8*			m_nOutputBlocks[NUM_BUFS];

	bool			m_bPrimed;		// true if the first NUM_BUF blocks have been read into the buffer
	int				m_nAvailBlocks;	// Number of blocks available
	int				m_nOutBlock;
};

CddaInput::CddaInput( os::String zDevicePath )
{
	int i;
	m_zDevicePath = zDevicePath;
	m_hCdrom = 0;
	m_psToc = NULL;

	for( i = 0; i < NUM_BUFS; i++ )
		m_nLastBlocks[i] = m_nTheseBlocks[i] = m_nOutputBlocks[i] = NULL;
}

CddaInput::~CddaInput()
{

}

os::String CddaInput::GetIdentifier()
{
	return( os::String( "CD Digital Audio (" ) + m_zDevicePath.substr( 14, 3 ) + os::String( ")" )  );
}

os::View* CddaInput::GetConfigurationView()
{
	return( NULL );
}

bool CddaInput::FileNameRequired()
{
	return( false );
}

status_t CddaInput::Open( os::String zFileName )
{
	m_hCdrom = open( m_zDevicePath.c_str(), O_RDONLY );
	if( m_hCdrom < 0 )
		return -1;

	/* Read the TOC */
	m_psToc = (cdrom_toc_s*)malloc( sizeof( cdrom_toc_s ));
	memset( m_psToc, 0, sizeof( cdrom_toc_s ));
	if( ioctl( m_hCdrom, CD_READ_TOC, m_psToc ) < 0 )
		return -1;

	/* Create buffer space */
	CreateBuffers();

	m_bPrimed = false;

	return 0;
}

void CddaInput::Close()
{
	int i;

	if( m_hCdrom > 0 )
		close( m_hCdrom );

	if( m_psToc != NULL )
		free( m_psToc );

	for( i = 0; i < NUM_BUFS; i++ )
	{
		if( m_nLastBlocks[i] != NULL )
		{
			free( m_nLastBlocks[i] );
			m_nLastBlocks[i] = NULL;
		}

		if( m_nTheseBlocks[i] != NULL )
		{
			free( m_nTheseBlocks[i] );
			m_nTheseBlocks[i] = NULL;
		}

		if( m_nOutputBlocks[i] != NULL )
		{
			free( m_nOutputBlocks[i] );
			m_nOutputBlocks[i] = NULL;
		}
	}
}

bool CddaInput::PacketBased()
{
	return( true );
}

bool CddaInput::StreamBased()
{
	return( false );
}

uint32 CddaInput::GetTrackCount()
{
	if( m_psToc == NULL )
		return 0;

	return m_psToc->hdr.last_track;
}

uint32 CddaInput::SelectTrack( uint32 nTrack )
{
	if( nTrack <= GetTrackCount() )
	{
		m_nSelected = nTrack;
		m_nCurentBlock = m_psToc->ent[m_nSelected].addr.lba;
		InvalidateBlockCache();
	}
	else
		m_nSelected = 0;

	return m_nSelected;
}

status_t CddaInput::ReadPacket( os::MediaPacket_s* psPacket )
{
	uint32 nBlock;
	int nError, i, j;
	struct cdda_block sInBlock;

	if( m_nCurentBlock >= m_psToc->ent[m_nSelected+1].addr.lba )
		return -1;

	if( m_bPrimed == false )
	{
		/* Read in NUM_BUFS blocks from block the first block of the track
		   and store them in last_blocks[] */
		for( i = 0; m_nCurentBlock < NUM_BUFS && m_nCurentBlock < m_psToc->ent[m_nSelected+1].addr.lba; m_nCurentBlock++, i++ )
		{
			sInBlock.nBlock = m_nCurentBlock;
			sInBlock.nSize = CD_CDDA_FRAMESIZE;
			sInBlock.pBuf = m_nLastBlocks[i];

			nError = ioctl( m_hCdrom, CD_READ_CDDA, &sInBlock );
			if( nError != 0 )
			{
				std::cout << "failed to read block " << m_nCurentBlock << std::endl;
				return -1;
			}
		}

		m_nAvailBlocks = 0;
		m_bPrimed = true;
	}

	if( m_nAvailBlocks == 0 )	// No blocks available.  Read more & jitter correct
	{
		uint32 nMaxBlocks;

		/* Seek back 4 blocks */
		m_nCurentBlock -= 4;

		nMaxBlocks = min( NUM_BUFS, m_psToc->ent[m_nSelected+1].addr.lba - m_nCurentBlock );
		for( nBlock = 0; nBlock < nMaxBlocks; nBlock++, m_nCurentBlock++ )
		{
			sInBlock.nBlock = m_nCurentBlock;
			sInBlock.nSize = CD_CDDA_FRAMESIZE;
			sInBlock.pBuf = m_nTheseBlocks[nBlock];

			nError = ioctl( m_hCdrom, CD_READ_CDDA, &sInBlock );
			if( nError != 0 )
			{
				std::cout << "failed to read block" << std::endl;
				return -1;
			}
		}

		/* Jitter correct */
		for( i = NUM_BUFS - OVERLAP; i < NUM_BUFS; i++ )
		{
			if( memcmp( m_nLastBlocks[i], m_nTheseBlocks[0], CD_CDDA_FRAMESIZE ) == 0 )
				break;
		}

		if( i == NUM_BUFS )
			std::cout << "Did not find overlap!" << std::endl;

		/* Add m_nLastBlocks[] from block 0 upto but not including block i to m_nOutputBlocks[] */
		for( j = 0; j < i; j++ )
			memcpy( m_nOutputBlocks[j], m_nLastBlocks[j], CD_CDDA_FRAMESIZE);

		m_nAvailBlocks = i;
		m_nOutBlock = 0;

		for( j = 0; j < NUM_BUFS; j++ )
			free( m_nLastBlocks[j] );

		/* Copy the buffer pointers from these_blocks[] to last_blocks[]
		   and re-create buffer space in these_blocks[]  */
		for( j = 0; j < NUM_BUFS; j++ )
		{
			m_nLastBlocks[j] = m_nTheseBlocks[j];
			m_nTheseBlocks[j] = (uint8*)malloc( CD_CDDA_FRAMESIZE );
		}			
	}

	psPacket->nStream = 0;
	psPacket->pBuffer[0] = ( uint8* )malloc( CD_CDDA_FRAMESIZE );
	memcpy( psPacket->pBuffer[0], m_nOutputBlocks[m_nOutBlock], CD_CDDA_FRAMESIZE );
	psPacket->nSize[0] = CD_CDDA_FRAMESIZE;
	psPacket->nTimeStamp = ~0;

	--m_nAvailBlocks;
	++m_nOutBlock;

	return 0;
}

void CddaInput::FreePacket( os::MediaPacket_s* psPacket )
{
	free( psPacket->pBuffer[0] );
}

uint64 CddaInput::GetLength()
{
	uint8 nMin, nSec, nFrame;
	uint64 nLength;

	lba_to_msf((m_psToc->ent[m_nSelected+1].addr.lba - m_psToc->ent[m_nSelected].addr.lba), &nMin, &nSec, &nFrame );
	nLength = (nMin * 60) + nSec;

	return nLength;
}

uint64 CddaInput::GetCurrentPosition()
{
	uint8 nMin, nSec, nFrame;
	uint64 nPosition;
	uint32 nBlockPosition = m_nCurentBlock - m_psToc->ent[m_nSelected].addr.lba;

	lba_to_msf(nBlockPosition, &nMin, &nSec, &nFrame );
	nPosition = (nMin * 60) + nSec;

	return nPosition;
}

uint64 CddaInput::Seek( uint64 nPosition )
{
	int nMin = 0, nSec = 0;
	uint32 nBlock;

	nMin = nPosition / 60;
	nSec = nPosition % 60;

	if( nMin > 0 || nSec > 0 )
	{
		nBlock = msf_to_lba(nMin, nSec, 0);
		m_nCurentBlock = m_psToc->ent[m_nSelected].addr.lba + nBlock;
		InvalidateBlockCache();
	}

	return nPosition;
}

uint32 CddaInput::GetStreamCount()
{
	return( 1 );
}

os::MediaFormat_s CddaInput::GetStreamFormat( uint32 nIndex )
{
	os::MediaFormat_s sFormat;
	
	sFormat.nType = os::MEDIA_TYPE_AUDIO;
	sFormat.zName = "Raw Audio";
	sFormat.nBitRate = 0;	// ???
	sFormat.nSampleRate = 44100;
	sFormat.nChannels = 2;
	sFormat.bVBR = false;

	return( sFormat );
}

void CddaInput::CreateBuffers()
{
	int i;

	for( i = 0; i < NUM_BUFS; i++ )
	{
		m_nLastBlocks[i] = (uint8*)malloc( CD_CDDA_FRAMESIZE );
		memset( m_nLastBlocks[i], 0, CD_CDDA_FRAMESIZE );
		m_nTheseBlocks[i] = (uint8*)malloc( CD_CDDA_FRAMESIZE );
		memset( m_nTheseBlocks[i], 0, CD_CDDA_FRAMESIZE );
		m_nOutputBlocks[i] = (uint8*)malloc( CD_CDDA_FRAMESIZE );
		memset( m_nOutputBlocks[i], 0, CD_CDDA_FRAMESIZE );
	}
}

void CddaInput::InvalidateBlockCache()
{
	int i;

	/* Remove current block data */
	for( i = 0; i < NUM_BUFS; i++ )
	{
		if( m_nLastBlocks[i] != NULL )
			free( m_nLastBlocks[i] );

		if( m_nTheseBlocks[i] != NULL )
			free( m_nTheseBlocks[i] );

		if( m_nOutputBlocks[i] != NULL )
			free( m_nOutputBlocks[i] );
	}

	/* Re-create buffer space */
	CreateBuffers();

	m_bPrimed = false;
}

class CddaAddon : public os::MediaAddon
{
public:
	status_t Initialize()
	{
		m_cDrives.clear();
		os::String zDrives[] = { "/dev/disk/ata/cda/raw",
							"/dev/disk/ata/cdb/raw",
							"/dev/disk/ata/cdc/raw",
							"/dev/disk/ata/cdd/raw",
							"/dev/disk/ata/cde/raw",
							"/dev/disk/ata/cdf/raw",
							"/dev/disk/ata/cdg/raw",
							"/dev/disk/ata/cdh/raw" };
							
		/* Try all cd drives */
		for( int i = 0; i < 8; i++ )
		{
			struct stat sStat;
			if( lstat( zDrives[i].c_str(), &sStat ) == 0 )
				m_cDrives.push_back( zDrives[i] );
		}
		return( 0 );
	}
	os::String GetIdentifier() { return( "CD Digital Audio" ); }
	uint32			GetCodecCount() { return( 0 ); }
	os::MediaCodec*		GetCodec( uint32 nIndex ) { return( NULL ); }
	uint32			GetInputCount() { return( m_cDrives.size() ); }
	os::MediaInput*		GetInput( uint32 nIndex ) { return( new CddaInput( m_cDrives[nIndex] ) ); }
private:
	std::vector<os::String> m_cDrives;
};

extern "C"
{
	os::MediaAddon* init_media_addon( os::String zDevice )
	{
		return( new CddaAddon() );
	}

}
