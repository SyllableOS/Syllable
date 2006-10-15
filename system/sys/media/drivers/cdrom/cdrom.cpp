/*  CDROM input plugin
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
 
#include <media/input.h>
#include <media/addon.h>
#include <atheos/cdrom.h>
#include <posix/ioctl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <iostream>
#include <vector>

class CDRom : public os::MediaInput
{
public:
	CDRom( os::String zDevicePath );
	~CDRom();
	os::String 		GetIdentifier();
		uint32			GetPhysicalType()
	{
		return( os::MEDIA_PHY_CD_DVD_ANALOG_IN );
	}
	os::View*		GetConfigurationView();
	
	bool			FileNameRequired();
	status_t 		Open( os::String zFileName );
	void 			Close();
	
	bool			PacketBased();
	bool			StreamBased();
	
	status_t		StartTrack();
	void			StopTrack();
	uint32			GetTrackCount();
	uint32			SelectTrack( uint32 nTrack );
	
	uint64			GetLength();
	uint64			GetCurrentPosition();
	uint64			Seek( uint64 nPosition );
	
	
private:
	os::String m_zDevicePath;
	cdrom_toc_s *m_psToc;
	int m_nTrack;
	int m_hDevice;
};

CDRom::CDRom( os::String zDevicePath )
{
	m_zDevicePath = zDevicePath;
	m_psToc = NULL;
	m_nTrack = 0;
	m_hDevice = -1;
}

CDRom::~CDRom()
{
	if( m_hDevice > 0 )
		close( m_hDevice );

	if( m_psToc != NULL )
		free( m_psToc );
}

os::String CDRom::GetIdentifier()
{
	return( os::String( "CD Audio (" ) + m_zDevicePath.substr( 14, 3 ) + os::String( ")" )  );
}

os::View* CDRom::GetConfigurationView()
{
	return( NULL );
}

bool CDRom::FileNameRequired()
{
	return( false );
}

status_t CDRom::Open( os::String zFileName )
{
	m_psToc = (cdrom_toc_s*)malloc( sizeof( cdrom_toc_s ));
	memset( m_psToc, 0, sizeof( cdrom_toc_s ));
	m_hDevice = open( m_zDevicePath.c_str(), O_RDONLY );
	if( m_hDevice >= 0 )
		if( ioctl( m_hDevice, CD_READ_TOC, m_psToc ) >= 0 ) {
		}
		else {
			close( m_hDevice );
			m_hDevice = -1;
		}
	if( m_hDevice < 0 )
		return( -1 );
	
	return( 0 );
}

void CDRom::Close()
{
	if( m_hDevice > 0 )
		close( m_hDevice );

	if( m_psToc != NULL )
	{
		free( m_psToc );
		m_psToc = NULL;
	}
	StopTrack();
}

bool CDRom::PacketBased()
{
	return( false );
}


bool CDRom::StreamBased()
{
	return( false );
}

status_t CDRom::StartTrack()
{
	cdrom_audio_track_s sCdTrack;
	uint8 nM, nS, nF;

	if(m_psToc != NULL )
		if( m_nTrack > 0 && m_nTrack <= m_psToc->hdr.last_track )
		{
			sCdTrack.track = m_nTrack;

			
			lba_to_msf( m_psToc->ent[sCdTrack.track-1].addr.lba, &nM, &nS, &nF );
			sCdTrack.msf_start.minute = nM;
			sCdTrack.msf_start.second = nS;
			sCdTrack.msf_start.frame = nF;
			

			lba_to_msf( m_psToc->ent[sCdTrack.track].addr.lba, &nM, &nS, &nF );
			sCdTrack.msf_stop.minute = nM;
			sCdTrack.msf_stop.second = nS;
			sCdTrack.msf_stop.frame = nF;

			ioctl( m_hDevice, CD_PLAY_MSF, &sCdTrack );
			return( 0 );
		}

	return( -1 );
}

void CDRom::StopTrack()
{
	ioctl( m_hDevice, CD_STOP );
}

uint32 CDRom::GetTrackCount()
{
	if( m_psToc != NULL )
		return( m_psToc->hdr.last_track );

	return( 0 );
}

uint32 CDRom::SelectTrack( uint32 nTrack )
{
	m_nTrack = nTrack + 1;
	return( m_nTrack - 1 );
}


uint64 CDRom::GetLength()
{
	uint64 nLength = 0;
	if( m_psToc != NULL )
		if( m_nTrack > 0 && m_nTrack <= m_psToc->hdr.last_track )
		{
			uint8 nM, nS, nF;
			lba_to_msf( m_psToc->ent[m_nTrack].addr.lba - m_psToc->ent[m_nTrack-1].addr.lba, &nM, &nS, &nF);
			nLength = ( nM * 60 ) + nS;
		}

	return( nLength );
}


uint64 CDRom::GetCurrentPosition()
{
	cdrom_msf_s sTime;
	uint64 nTrackStartTime, nTime = 0;
	uint8 nM, nS, nF;

	if( m_psToc != NULL ) {
	lba_to_msf( m_psToc->ent[m_nTrack-1].addr.lba, &nM, &nS, &nF );
	nTrackStartTime = ( nM * 60 ) + nS;

	ioctl( m_hDevice, CD_GET_TIME, &sTime );
	
	if( ( ( sTime.minute - nM ) * 60 ) + ( sTime.second - nS ) < 0 )
		nTime = 0;
	else
		nTime = ( ( sTime.minute - nM ) * 60 ) + ( sTime.second - nS ) /*- nTrackStartTime*/;
	}
	return( nTime );
}

uint64 CDRom::Seek( uint64 nPosition )
{
	cdrom_audio_track_s sCdTrack;
	uint8 nM, nS, nF;
	int nLba;
	sCdTrack.track = m_nTrack;

	if( m_psToc != NULL )
	{
		nM = nPosition / 60;
		nS = nPosition - ( nM * 60 );
		nLba = msf_to_lba( nM, nS, 0 );
		/* Is this right ? */
		if( nLba < 0 )
			nLba = 0;
		
		nLba += m_psToc->ent[sCdTrack.track-1].addr.lba;

		lba_to_msf( nLba, &nM, &nS, &nF );
		sCdTrack.msf_start.minute = nM;
		sCdTrack.msf_start.second = nS;
		sCdTrack.msf_start.frame = nF;

		lba_to_msf( m_psToc->ent[sCdTrack.track].addr.lba, &nM, &nS, &nF );
		sCdTrack.msf_stop.minute = nM;
		sCdTrack.msf_stop.second = nS;
		sCdTrack.msf_stop.frame = nF;
		ioctl( m_hDevice, CD_PLAY_MSF, &sCdTrack );
		return( nPosition );
	}
	return( 0 );
}


class CDRomAddon : public os::MediaAddon
{
public:
	status_t Initialize()
	{
		m_cDrives.clear();
		os::String zDrives[] = { "/dev/disk/ide/cda/raw",
							"/dev/disk/ide/cdb/raw", 
							"/dev/disk/ide/cdc/raw",
							"/dev/disk/ide/cdd/raw",
							"/dev/disk/ata/cda/raw",
							"/dev/disk/ata/cdb/raw",
							"/dev/disk/ata/cdc/raw",
							"/dev/disk/ata/cdd/raw" };
							
		/* Try all cd drives */
		for( int i = 0; i < 8; i++ )
		{
			struct stat sStat;
			if( lstat( zDrives[i].c_str(), &sStat ) == 0 )
				m_cDrives.push_back( zDrives[i] );
		}
		return( 0 );
	}
	os::String GetIdentifier() 
	{
		return( "CD Audio" );
	}
	uint32 GetInputCount()
	{ 
		return( m_cDrives.size() );
	}
	os::MediaInput*	GetInput( uint32 nIndex ) 
	{
		return( new CDRom( m_cDrives[nIndex] ) );
	}
private:
	std::vector<os::String> m_cDrives;
};


extern "C"
{
	os::MediaAddon* init_media_addon()
	{
		return( new CDRomAddon() );
	}

}

