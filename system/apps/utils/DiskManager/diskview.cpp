/*
 *  DiskManager - AtheOS disk partitioning tool.
 *  Copyright (C) 2000 Kurt Skauen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <gui/listview.h>
#include <gui/button.h>
#include <util/message.h>

#include "diskview.h"
#include "partitionview.h"
#include "main.h"
#include "resources/DiskManager.h"

#include <stdexcept>

using namespace os;

enum {
    ID_NEW_SELECTION,
    ID_PARTITION
};

typedef struct
{
    uint8  p_nStatus;
    uint8  p_nFirstHead;
    uint16 p_nFirstCyl;
    uint8  p_nType;
    uint8  p_nLastHead;
    uint16 p_nLastCyl;
    uint32 p_nStartLBA;
    uint32 p_nSize;
} PartitionRecord_s;

class DiskListRow : public ListViewStringRow
{
public:
    DiskListRow( const DiskInfo& cInfo ) { m_cDiskInfo = cInfo; }
    DiskInfo	m_cDiskInfo;
};

#if 0
static int decode_partitions( int nDevice, DiskInfo* psDisk )
{
    char anBuffer[512];
    PartitionRecord_s* pasTable = (PartitionRecord_s*)(anBuffer + 0x1be);
    int	i;
    int	nCount = 0;
    off_t nDiskSize = psDisk->m_sDriveInfo.sector_count * psDisk->m_sDriveInfo.bytes_per_sector;
    off_t nTablePos = psDisk->m_nOffset;
    off_t nExtStart = 0;
    off_t nExtSize  = 0;
    int	  nExtPart  = 0;
    int	  nNumExtended;
    int	  nNumActive;
    
repeat:
    lseek( nDevice, SEEK_SET, nTablePos );
    if ( read( nDevice, anBuffer, 512 ) != 512 ) {
	printf( "Error: decode_partitions() failed to read MBR\n" );
	return( -EIO );
    }
    if ( *((uint16*)(anBuffer + 0x1fe)) != 0xaa55 ) {
	printf( "Error: decode_partitions() Invalid partition table signature %04x\n", *((uint16*)(anBuffer + 0x1fe)) );
	return( -EINVAL );
    }

    nNumActive   = 0;
    nNumExtended = 0;
    
    for ( i = 0 ; i < 4 ; ++i ) {
	if ( pasTable[i].p_nStatus & 0x80 ) {
	    nNumActive++;
	}
	if ( pasTable[i].p_nType == 0x05 || pasTable[i].p_nType == 0x0f || pasTable[i].p_nType == 0x85 ) {
	    nNumExtended++;
	}
	if ( nNumActive > 1 ) {
	    printf( "Warning: decode_disk_partitions() more than one active partitions\n" );
//	    return( -EINVAL );
	}
	if ( nNumExtended > 1 ) {
	    printf( "Warning: decode_disk_partitions() more than one extended partitions\n" );
//	    return( -EINVAL );
	}
    }
    for ( i = 0 ; i < 4 ; ++i ) {
	PartitionInfo& sPartition = psDisk->m_asPartitions[i];

	sprintf( sPartition.m_zName, "%d", nCount );
	
    
	if ( pasTable[i].p_nType == 0 ) {
	    sPartition.m_nType = 0;
	    sPartition.m_nSize = 0;
	    sPartition.m_nStart = 0;
	}
	if ( pasTable[i].p_nType == 0x05 || pasTable[i].p_nType == 0x0f || pasTable[i].p_nType == 0x85 ) {
	    nExtStart = ((uint64)pasTable[i].p_nStartLBA) * ((uint64)psDisk->m_sDriveInfo.bytes_per_sector) + nTablePos;
	    nExtSize  = ((uint64)pasTable[i].p_nSize) * ((uint64)psDisk->m_sDriveInfo.bytes_per_sector);
	    nExtPart  = i;
	    if ( nExtStart + nExtSize - nTablePos > nDiskSize ) {
		printf( "Warning: Extended partition %d extend outside the disk/extended partition\n", nCount );
//		return( -EINVAL );
	    }
//	    continue;
	}
//	PartitionInfo sPartition;
	sPartition.m_nType   = pasTable[i].p_nType;
	sPartition.m_nStatus = pasTable[i].p_nStatus;
	sPartition.m_nStart  = ((uint64)pasTable[i].p_nStartLBA) * ((uint64)psDisk->m_sDriveInfo.bytes_per_sector) + nTablePos;
	sPartition.m_nSize   = ((uint64)pasTable[i].p_nSize) * ((uint64)psDisk->m_sDriveInfo.bytes_per_sector);


//	psDisk->m_cPartitions.push_back( sPartition );
	
	printf( "Partition %d : %10Ld -> %10Ld %02x (%Ld)\n", nCount, sPartition.m_nStart,
		sPartition.m_nStart + sPartition.m_nSize - 1LL, sPartition.m_nType, sPartition.m_nSize );

	if ( sPartition.m_nStart + sPartition.m_nSize - nTablePos > nDiskSize ) {
	    printf( "Warning: Partition %d extend outside the disk/extended partition\n", nCount );
	}
	  /*
	for ( j = 0 ; j < nCount ; ++j ) {
	    if ( pasPartitions[j].p_nStart + pasPartitions[j].p_nSize > pasPartitions[nCount].p_nStart &&
		 pasPartitions[j].p_nStart <  pasPartitions[nCount].p_nStart + pasPartitions[nCount].p_nSize ) {
		printf( "Warning: decode_disk_partitions() partition %d overlap partition %d\n", j, nCount );
	    }
	}*/
	nCount++;
    }
    if ( nExtStart != 0 ) {
	printf( "Extended partition at %Ld\n", nExtStart );
	nTablePos = nExtStart;
	nDiskSize = nExtSize;
	nExtStart = 0;
	nExtSize  = 0;

	psDisk->m_asPartitions[nExtPart].m_psExtended = new DiskInfo;
	
	*psDisk->m_asPartitions[nExtPart].m_psExtended = *psDisk;
	psDisk->m_asPartitions[nExtPart].m_psExtended->m_psParent = &psDisk->m_asPartitions[nExtPart];
	psDisk->m_asPartitions[nExtPart].m_psExtended->m_nOffset = nExtStart;
	psDisk->m_asPartitions[nExtPart].m_psExtended->m_sDriveInfo.sector_count = nExtSize / psDisk->m_sDriveInfo.bytes_per_sector;
	
	
	if ( nCount < 4 ) {
	    nCount = 4;
	}
	goto repeat;
    }
//done:
    return( nCount );
}
#endif

static void add_disk( std::vector<DiskInfo>* pcList, const std::string& cPath )
{
    DiskInfo cInfo;

    cInfo.m_nDiskFD = open( cPath.c_str(), O_RDONLY );
    if ( cInfo.m_nDiskFD < 0 ) {
	return;
    }
    if ( ioctl( cInfo.m_nDiskFD, IOCTL_GET_DEVICE_GEOMETRY, &cInfo.m_sDriveInfo ) < 0 ) {
	close( cInfo.m_nDiskFD );
	return;
    }
    cInfo.m_cPath = cPath;
    pcList->push_back( cInfo );
}

static void find_drives( std::vector<DiskInfo>* pcList, const std::string& cPath )
{
    DIR* hDir = opendir( cPath.c_str() );
    if ( hDir == NULL ) {
	return;
    }
    struct dirent* psEntry;
    while( (psEntry = readdir(hDir)) != NULL ) {
	if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 ) {
	    continue;
	}
	std::string cFullPath = cPath + psEntry->d_name;
	struct stat sStat;
	if ( stat( cFullPath.c_str(), &sStat ) < 0 ) {
	    continue;
	}
	if ( S_ISDIR( sStat.st_mode ) ) {
	    cFullPath += '/';
	    find_drives( pcList, cFullPath );
	} else if ( strcmp( psEntry->d_name, "raw" ) == 0 ) {
	    add_disk( pcList, cFullPath );
	}
    }
    closedir( hDir );
}

DiskView::DiskView( const Rect& cFrame, const std::string& cName ) : LayoutView( cFrame, cName )
{
    HLayoutNode* pcRoot = new HLayoutNode( "root" );
    VLayoutNode* pcButtonFrame = new VLayoutNode( "button_frame" );
    
    m_pcDiskList = new ListView( Rect(), "disk_list", ListView::F_RENDER_BORDER );
    m_pcPartitionButton = new Button( Rect(), "partition_but", MSG_MAINWND_BUTONS_PARTITION, new Message( ID_PARTITION ) );
    m_pcQuitButton = new Button( Rect(), "quit_but", MSG_MAINWND_BUTONS_QUIT, new Message( M_QUIT ) );

    m_pcDiskList->InsertColumn( MSG_MAINWND_LIST_DEVICE.c_str(), 150 );
    m_pcDiskList->InsertColumn( MSG_MAINWND_LIST_PARTITIONS.c_str(), 100 );
    m_pcDiskList->InsertColumn( MSG_MAINWND_LIST_SIZE.c_str(), 100 );
    m_pcDiskList->SetSelChangeMsg( new Message( ID_NEW_SELECTION ) );
	
    m_pcPartitionButton->SetEnable( false );

    pcButtonFrame->AddChild( new LayoutSpacer( "space", 1.0f, NULL, Point(0.0f,0.0f), Point(0.0f,MAX_SIZE) ) );
    pcButtonFrame->AddChild( m_pcPartitionButton, 0.0f );
    pcButtonFrame->AddChild( m_pcQuitButton, 0.0f );

    pcRoot->AddChild( m_pcDiskList );
    pcRoot->AddChild( pcButtonFrame );

    std::vector<DiskInfo> cDiskInfoList;
    find_drives( &cDiskInfoList, "/dev/disk/" );

    for ( uint i = 0 ; i < cDiskInfoList.size() ; ++i ) {
	ListViewStringRow* pcRow = new DiskListRow( cDiskInfoList[i] );
	pcRow->AppendString( cDiskInfoList[i].m_cPath );
	pcRow->AppendString( "" );
	char zSize[64];
	sprintf( zSize, MSG_MAINWND_LIST_MB.c_str(),
		 double(cDiskInfoList[i].m_sDriveInfo.sector_count * cDiskInfoList[i].m_sDriveInfo.bytes_per_sector) / (1024.0*1024.0) );
	pcRow->AppendString( zSize );
	m_pcDiskList->InsertRow( pcRow, false );
    }
    
    pcRoot->SameWidth( "partition_but", "quit_but", NULL );
    pcRoot->SetBorders( Rect( 10.0f, 10.0f, 10.0f, 5.0f ), "partition_but", "quit_but", NULL );
    
    SetRoot( pcRoot );
}

void DiskView::AllAttached()
{
    m_pcDiskList->SetTarget( this );
    m_pcPartitionButton->SetTarget( this );
}

void DiskView::Paint( const Rect& cUpdateRect )
{
    LayoutView::Paint( cUpdateRect );
}

void DiskView::HandleMessage( Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_NEW_SELECTION:
	    m_pcPartitionButton->SetEnable( m_pcDiskList->GetFirstSelected() != -1 );
	    break;
	case ID_PARTITION:
	{
	    try {
		DiskListRow *pcRow = dynamic_cast<DiskListRow*>( m_pcDiskList->GetRow( m_pcDiskList->GetFirstSelected() ) );
		if ( pcRow != NULL ) {
		    Window* pcReq = new PartitionEditReq( pcRow->m_cDiskInfo );
		    pcReq->Show();
		}
	    } catch ( std::runtime_error& cExc ) {
		printf( "Failed to open partition editior: %s\n", cExc.what() );
	    }
	    break;
	}
	default:
	    LayoutView::HandleMessage( pcMessage );
	    break;
    }
}


void DiskView::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
}

void DiskView::MouseDown( const Point& cPosition, uint32 nButtons )
{
}

void DiskView::MouseUp( const Point& cPosition, uint32 nButtons, Message* pcData )
{
}

void DiskView::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
}

void DiskView::KeyUp( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
}

