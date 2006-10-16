#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <atheos/filesystem.h>

#include <gui/spinner.h>
#include <gui/checkbox.h>
#include <gui/dropdownmenu.h>
#include <gui/button.h>
#include <gui/stringview.h>
#include <gui/desktop.h>
#include <gui/frameview.h>
#include <gui/requesters.h>

#include <util/application.h>
#include <util/message.h>

#include <algorithm>
#include <stdexcept>
#include <cassert>

#include "partitionview.h"
#include "main.h"
#include "resources/DiskManager.h"

using namespace os;

enum
{
    ID_OK,
    ID_CANCEL,
    ID_PARTITION_EXTENDED,
    ID_CLEAR,
    ID_END_EXTENDED_EDIT,
    ID_START_CHANGED,
    ID_END_CHANGED,
    ID_PARTITION_STATUS,
    ID_TYPE_SELECTED,
    ID_TYPE_EDITED,
    ID_DELETE_PARTITION,
    ID_WRITE_PARTITION_TABLE,
    ID_SORT,
    ID_ADD,
    ID_PREV,
    ID_NEXT
};

struct PNode {
    PNode( int s, int e ) { m_nStart = s; m_nEnd = e; }
    int m_nStart;
    int m_nEnd;
    bool operator<( const PNode& cNode ) const { return( m_nStart < cNode.m_nStart ); }
};

class ComparePartitions {
public:
    bool operator()( const PartitionEntry* p1, const PartitionEntry* p2 ) const { return( p1->m_nStart < p2->m_nStart ); }
};

struct PartitionType
{
	const int pt_nType;			// Partition type ID
	const char* pt_pzName;		// Associated file system name E.g. "AFS", "FAT32", "ext2"
	const char* pt_pzSystem;	// "Owning" OS E.g. "Syllable", "Windows"
	uint32 pt_nColor;			// RGB32 Color to drawn the partition bar with
};
  
PartitionType g_asPartitionTypes[] =
{
	{ 0x00, "Unused", "", COL_TO_RGB32( Color32_s( 225, 225, 225, 0 ) ) },
	{ 0x2a, "Syllable", "AFS", COL_TO_RGB32( Color32_s( 150, 190, 255, 0 ) ) },
	{ 0xeb, "BeOS", "BeFS", COL_TO_RGB32( Color32_s( 255, 255, 0, 0 ) ) },
	{ 0x83, "Linux", "", COL_TO_RGB32( Color32_s( 255, 0, 0, 0 ) ) },
	{ 0x82, "Linux", "Swap", COL_TO_RGB32( Color32_s( 150, 0, 0, 0 ) ) },
	{ 0x07, "Windows", "NTFS", COL_TO_RGB32( Color32_s( 0, 170, 0, 0 ) ) },
	{ 0x0c, "Windows", "FAT32 (LBA)", COL_TO_RGB32( Color32_s( 0, 0, 255, 0 ) ) },
	{ 0x05, "Extended", "", COL_TO_RGB32( Color32_s( 125, 125, 125, 0 ) ) },
	{ 0x8e, "Linux", "LVM", COL_TO_RGB32( Color32_s( 200, 0, 0, 0 ) ) },
	{ 0xa5, "FreeBSD", "", COL_TO_RGB32( Color32_s( 255, 0, 255, 0 ) ) },
	{ 0xa6, "OpenBSD", "", COL_TO_RGB32( Color32_s( 0, 255, 255, 0 ) ) },
	{ 0xa9, "NetBSD", "", COL_TO_RGB32( Color32_s( 255, 150, 0, 0 ) ) },
	{ 0x0b, "Windows", "FAT32", COL_TO_RGB32( Color32_s( 0, 0, 200, 0 ) ) },
	{ 0x0e, "Windows", "FAT16 (LBA)", COL_TO_RGB32( Color32_s( 0, 255, 0, 0 ) ) },
	{ 0x06, "Windows", "FAT16", COL_TO_RGB32( Color32_s( 0, 200, 0, 0 ) ) }
};

#define H_POINTER_WIDTH  20
#define H_POINTER_HEIGHT 9
static uint8 g_anHMouseImg[]=
{
    0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x03,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x02,0x03,0x00,0x00,0x00,
    0x00,0x00,0x03,0x02,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x02,0x02,0x03,0x00,0x00,
    0x00,0x03,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x02,0x02,0x02,0x03,0x00,
    0x03,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x03,
    0x00,0x03,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x02,0x02,0x02,0x03,0x00,
    0x00,0x00,0x03,0x02,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x02,0x02,0x03,0x00,0x00,
    0x00,0x00,0x00,0x03,0x02,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x02,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x00,0x00,
};

// Some idiots figured out it was much better with 3 ways to mark a
// partition as "extended" than 1! <sigh>

#define IS_EXTENDED(t) ((t) == 0x05 || (t) == 0x0f || (t) == 0x85)


typedef std::map<std::string,fs_info> MountMap_t;


static void lba_to_csh( uint32 nLBA, const device_geometry& sGeom, uint8* pnCL, uint8* pnCH, uint8* pnDH )
{
    uint32 nCylinder;
    uint32 nHead;
    uint32 nSector;

    nSector   = nLBA % sGeom.sectors_per_track + 1;
    nHead     = (nLBA / sGeom.sectors_per_track) % sGeom.head_count;
    nCylinder = nLBA / (sGeom.sectors_per_track * sGeom.head_count);

    if (nCylinder >= (sGeom.cylinder_count & 0x3ff)) {
	nCylinder = sGeom.cylinder_count - 1;
	*pnCL = 255;
	*pnCH = 255;
	*pnDH = 255;
    } else {
	*pnCL = nSector | ((nCylinder & 0x300) >> 2);
	*pnCH = nCylinder & 0xFF;
	*pnDH = nHead;
    }
}

static void load_logical( PartitionList_t* pcList, MountMap_t* pcMountMap, const DiskInfo& cDisk, off_t nOffset, off_t nRealOff = 0 )
{
    BootBlock_s sBootBlock;

    std::string cDevicePath = std::string( cDisk.m_cPath.begin(), cDisk.m_cPath.end() - 3 );
    
    if ( read_pos( cDisk.m_nDiskFD, (nOffset + nRealOff) * cDisk.m_sDriveInfo.bytes_per_sector, &sBootBlock, sizeof(sBootBlock) ) != 512 ) {
	return;
    }
    if ( sBootBlock.p_nSignature != 0xaa55 ) {
	return; // Invalid extended partition
    }

    int nExtendedCount = 0;
    int nLogicalCount  = 0;
    
    for ( int i = 0 ; i < 4 ; ++i ) {
	if ( IS_EXTENDED( sBootBlock.bb_asPartitions[i].p_nType ) ) {
	    nExtendedCount++;
	} else if ( sBootBlock.bb_asPartitions[i].p_nType != 0 ) {
	    nLogicalCount++;
	}
    }
    if ( nExtendedCount > 1 ) {
	(new Alert( MSG_ALERTWND_ERREXT_EXTENDED, MSG_ALERTWND_ERREXT_EXTENDED_TEXT,Alert::ALERT_INFO, 0, MSG_ALERTWND_ERREXT_EXTENDED_OK.c_str(), NULL ))->Go();
	return;
    }
    if ( nLogicalCount > 1 ) {
	(new Alert( MSG_ALERTWND_ERREXT_LOGICAL, MSG_ALERTWND_ERREXT_LOGICAL_TEXT,Alert::ALERT_INFO, 0, MSG_ALERTWND_ERREXT_LOGICAL_OK.c_str(), NULL ))->Go();
	return;
    }
    
    for ( int i = 0 ; i < 4 ; ++i ) {
	if ( sBootBlock.bb_asPartitions[i].p_nType == 0 ) {
	    continue;
	}
	if ( IS_EXTENDED( sBootBlock.bb_asPartitions[i].p_nType ) ) {
	    load_logical( pcList, pcMountMap, cDisk, nOffset, sBootBlock.bb_asPartitions[i].p_nStart );
	} else {
	    PartitionEntry* pcEntry = new PartitionEntry;
	    pcEntry->m_nType   = sBootBlock.bb_asPartitions[i].p_nType;
	    pcEntry->m_nStatus = sBootBlock.bb_asPartitions[i].p_nStatus;
	    pcEntry->m_nStart  = (sBootBlock.bb_asPartitions[i].p_nStart + nRealOff) / cDisk.m_sDriveInfo.sectors_per_track;
	    pcEntry->m_nEnd    = (sBootBlock.bb_asPartitions[i].p_nStart + nRealOff) / cDisk.m_sDriveInfo.sectors_per_track +
		sBootBlock.bb_asPartitions[i].p_nSize / cDisk.m_sDriveInfo.sectors_per_track - 1;

	    pcEntry->m_nOrigStart  = pcEntry->m_nStart;
	    pcEntry->m_nOrigEnd    = pcEntry->m_nEnd;
	    pcEntry->m_nOrigType   = pcEntry->m_nType;
	    pcEntry->m_nOrigStatus = pcEntry->m_nStatus;
	    
	    char zName[16];
	    sprintf( zName, "%d", pcList->size() + 4 );
	    pcEntry->m_cPartitionPath = cDevicePath + zName;

	    fs_info sInfo;
	    MountMap_t::iterator cMI = pcMountMap->find( pcEntry->m_cPartitionPath );
	    if ( cMI != pcMountMap->end() ) {
		pcEntry->m_cVolumeName = (*cMI).second.fi_volume_name;
		pcEntry->m_bIsMounted = true;
	    } else {
		pcEntry->m_bIsMounted = false;
		if ( probe_fs( pcEntry->m_cPartitionPath.c_str(), &sInfo ) >= 0 ) {
		    pcEntry->m_cVolumeName = sInfo.fi_volume_name;
		} else {
		    pcEntry->m_cVolumeName = MSG_PARTWND_PARTINFO_UNKNOWN;
		}
	    }
	    pcList->push_back( pcEntry );
	}
    }
}




static PartitionTable* load_ptable( PartitionTable* pcParent, MountMap_t* pcMountMap, const DiskInfo& cDisk, off_t nOffset, off_t nSize )
{
    PartitionTable* pcTable = new PartitionTable;

    pcTable->m_cDevicePath = std::string( cDisk.m_cPath.begin(), cDisk.m_cPath.end() - 3 );
    pcTable->m_nPosition = nOffset;
    pcTable->m_nSize     = nSize;

    if ( pcParent != NULL ) {
	pcTable->m_nPosition -= pcParent->m_nPosition;
    }
    
    pcTable->m_nNestingLevel = (pcParent != NULL) ? (pcParent->m_nNestingLevel + 1) : 0;

    
    if ( read_pos( cDisk.m_nDiskFD, nOffset, &pcTable->m_sBootBlock, sizeof(pcTable->m_sBootBlock) ) != 512 ) {
	delete pcTable;
	return( NULL );
    }
    if ( pcTable->m_sBootBlock.p_nSignature != 0xaa55 ) {
	if ( pcParent == NULL ) {
	    pcTable->m_sBootBlock.p_nSignature = 0xaa55;
	    memset( &pcTable->m_sBootBlock.bb_asPartitions, 0, sizeof( pcTable->m_sBootBlock.bb_asPartitions ) );
	} else {
	    printf( "Bad signature\n" );
	    delete pcTable;
	    return( NULL );
	}
    }
    
    for ( int i = 0 ; i < 4 ; ++i ) {
	char zName[16];
	sprintf( zName, "%d", pcTable->m_nNestingLevel * 4 + i );
	pcTable->m_acPartitionPaths[i] = pcTable->m_cDevicePath + zName;

	pcTable->m_abMountedFlags[i] = false;

	if ( IS_EXTENDED( pcTable->m_sBootBlock.bb_asPartitions[i].p_nType ) ) {
	    load_logical( &pcTable->m_cLogicalPartitions, pcMountMap, cDisk, pcTable->m_sBootBlock.bb_asPartitions[i].p_nStart, 0 );
	    for ( uint j = 0 ; j < pcTable->m_cLogicalPartitions.size() ; ++j ) {
		if ( pcTable->m_cLogicalPartitions[j]->m_bIsMounted ) {
		    pcTable->m_abMountedFlags[i] = true;
		    break;
		}
	    }
	} else {
	    if ( pcTable->m_sBootBlock.bb_asPartitions[i].p_nType != 0 ) {
		fs_info sInfo;
		MountMap_t::iterator cMI = pcMountMap->find( pcTable->m_acPartitionPaths[i] );
		if ( cMI != pcMountMap->end() ) {
		    pcTable->m_acVolumeNames[i] = (*cMI).second.fi_volume_name;
		    pcTable->m_abMountedFlags[i] = true;
		} else {
		    if ( probe_fs( pcTable->m_acPartitionPaths[i].c_str(), &sInfo ) >= 0 ) {
			pcTable->m_acVolumeNames[i] = sInfo.fi_volume_name;
		    } else {
			pcTable->m_acVolumeNames[i] = MSG_PARTWND_PARTINFO_UNKNOWN;
		    }
		}
	    }
	}
    }
    return( pcTable );
}

static void build_mount_map( MountMap_t* pcMap )
{
    int nNumMntPoints = get_mount_point_count();

    for ( int i = 0 ; i < nNumMntPoints ; ++i ) {
	char    zPath[PATH_MAX];
	fs_info sInfo;
	if ( get_mount_point( i, zPath, PATH_MAX ) < 0 ) {
	    continue;
	}
	int nRootFD = open( zPath, O_RDONLY );
	if ( nRootFD < 0 ) {
	    continue;
	}
	if ( get_fs_info( nRootFD, &sInfo ) >= 0 ) {
	    (*pcMap)[sInfo.fi_device_path] = sInfo;
	}
	close( nRootFD );
    }
}

off_t PartitionEntry::GetByteSize( const device_geometry& sDiskGeom ) const
{
    return( (m_nEnd - m_nStart + 1) * sDiskGeom.sectors_per_track * sDiskGeom.bytes_per_sector );
}

/******************************************************************************
**** PartitionEditReq *********************************************************
******************************************************************************/

PartitionEditReq::PartitionEditReq( const DiskInfo& cDisk ) : Window( Rect(0,0,0,0), "partition_edit", MSG_PARTWND_TITLE )
{
    IPoint cScreenRes;
    m_cDiskInfo = cDisk;

    m_bLayoutChanged = false;
    m_bTypeChanged   = false;
    m_bStatusChanged = false;

    m_bIsPrimaryChanged = false;
    m_bIsExtendedChanged = false;
    
    
      // Need a new scope to reduce the time the desktop is locked.
    { cScreenRes = Desktop().GetResolution();  }

    MountMap_t cMountMap;

    build_mount_map( &cMountMap );

    m_pcFirstPTable = load_ptable( NULL, &cMountMap, cDisk, 0, cDisk.m_sDriveInfo.sector_count * cDisk.m_sDriveInfo.bytes_per_sector );

    if ( m_pcFirstPTable == NULL ) {
	throw std::runtime_error( "Failed to read the partition table" );
    }

    m_nExtendedPos = 0;
    
    for ( int i = 0 ; i < 4 ; ++i ) {
	PartitionEntry* pcEntry = new PartitionEntry;
	pcEntry->m_nType   = m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nType;
	pcEntry->m_nStatus = m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nStatus;
	pcEntry->m_nStart  = m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nStart / m_cDiskInfo.m_sDriveInfo.sectors_per_track;
	pcEntry->m_nEnd	   = m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nStart / m_cDiskInfo.m_sDriveInfo.sectors_per_track +
	    m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nSize / m_cDiskInfo.m_sDriveInfo.sectors_per_track - 1;

	pcEntry->m_nOrigStart  = pcEntry->m_nStart;
	pcEntry->m_nOrigEnd    = pcEntry->m_nEnd;
	pcEntry->m_nOrigType   = pcEntry->m_nType;
	pcEntry->m_nOrigStatus = pcEntry->m_nStatus;
	
	pcEntry->m_bIsMounted     = m_pcFirstPTable->m_abMountedFlags[i];
	pcEntry->m_cPartitionPath = m_pcFirstPTable->m_acPartitionPaths[i];
	pcEntry->m_cVolumeName    = m_pcFirstPTable->m_acVolumeNames[i];

	if ( m_nExtendedPos == 0 && IS_EXTENDED( pcEntry->m_nType ) ) {
	    m_nExtendedPos = pcEntry->m_nStart;
	}
	m_cPrimary.push_back( pcEntry );
    }
    
    PartitionEdit* pcEditView = new PartitionEdit( Rect(0,0,0,0), "edit", cDisk, &m_cPrimary,
						   m_cDiskInfo.m_sDriveInfo.sector_count / m_cDiskInfo.m_sDriveInfo.sectors_per_track, false );

    m_cEditViews.push( pcEditView );
    AddChild( pcEditView );

    Point cMinSize = pcEditView->GetPreferredSize(false);
    Point cMaxSize = pcEditView->GetPreferredSize(true);

    Rect cFrame( 0, 0, cMinSize.x - 1.0f, cMinSize.y - 1.0f );
    cFrame += Point( cScreenRes.x / 2 - (cFrame.Width()+1.0f) / 2, cScreenRes.y / 2 - (cFrame.Height()+1.0f) / 2 );
    cFrame.Floor();
    SetFrame( cFrame );
    
    SetSizeLimits( cMinSize, cMaxSize );
    
    MakeFocus();
}

void PartitionEditReq::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_PARTITION_EXTENDED:
	{
	    if ( m_cEditViews.size() > 1 ) {
		break;
	    }
	    off_t nOffset;
	    off_t nSize;
	    int32 nPIndex;
	    
	    if ( pcMessage->FindInt32( "pindex", &nPIndex ) != 0 ) {
		break;
	    }
	    if ( pcMessage->FindInt64( "offset", &nOffset ) != 0 ) {
		break;
	    }
	    if ( pcMessage->FindInt64( "size", &nSize ) != 0 ) {
		break;
	    }
	    if ( nOffset != m_nExtendedPos ) {
		for ( uint i = 0 ; i < m_pcFirstPTable->m_cLogicalPartitions.size() ; ++i ) {
		    delete m_pcFirstPTable->m_cLogicalPartitions[i];
		}
		m_pcFirstPTable->m_cLogicalPartitions.clear();
	    } else {
		for ( PartitionList_t::iterator i = m_pcFirstPTable->m_cLogicalPartitions.begin() ;
		      i != m_pcFirstPTable->m_cLogicalPartitions.end() ; ) {
		    if ( (*i)->m_nStart >= nSize ) {
			delete *i;
			i = m_pcFirstPTable->m_cLogicalPartitions.erase( i );
		    } else if ( (*i)->m_nEnd >= nSize ) {
			(*i)->m_nEnd = nSize - 1;
			++i;
		    } else {
			++i;
		    }
		}
		
	    }
	    m_nExtendedPos = nOffset;
	    
	    PartitionEdit* pcEditView = new PartitionEdit( GetBounds(), "edit", m_cDiskInfo, &m_pcFirstPTable->m_cLogicalPartitions,
							   nSize, true );

	    RemoveChild( m_cEditViews.top() );
	    m_cEditViews.push( pcEditView );
	    AddChild( pcEditView );
	    break;
	}
	case ID_END_EXTENDED_EDIT:
	{
	    PartitionEdit* pcEditView = m_cEditViews.top();

	    bool bDoSave = false;
	    pcMessage->FindBool( "do_save", &bDoSave );
	    
	    m_bTypeChanged = m_bTypeChanged || pcEditView->m_bTypeChanged;
	    m_bLayoutChanged = m_bLayoutChanged || pcEditView->m_bLayoutChanged;
	    m_bStatusChanged = m_bStatusChanged || pcEditView->m_bStatusChanged;
	    
	    if ( m_cEditViews.size() == 1 ) {
		if ( bDoSave && (m_bTypeChanged || m_bLayoutChanged || m_bStatusChanged) ) {

		      // Delete or truncate logical partitions if the extended partition has changed.
		    bool bHasExtended = false;
		    for ( uint i = 0 ; i < m_cPrimary.size() ; ++i ) {
			if ( IS_EXTENDED( m_cPrimary[i]->m_nType ) == false ) {
			    continue;
			}
			uint32 nSize = m_cPrimary[i]->m_nEnd - m_cPrimary[i]->m_nStart + 1;
			if ( m_cPrimary[i]->m_nStart != m_nExtendedPos ) {
			    for ( uint j = 0 ; j < m_pcFirstPTable->m_cLogicalPartitions.size() ; ++j ) {
				delete m_pcFirstPTable->m_cLogicalPartitions[j];
			    }
			    m_pcFirstPTable->m_cLogicalPartitions.clear();
			    m_bIsExtendedChanged = true;
			} else {
			    for ( PartitionList_t::iterator j = m_pcFirstPTable->m_cLogicalPartitions.begin() ;
				  j != m_pcFirstPTable->m_cLogicalPartitions.end() ; ) {
				if ( (*j)->m_nStart >= nSize ) {
				    delete *j;
				    j = m_pcFirstPTable->m_cLogicalPartitions.erase( j );
				    m_bIsExtendedChanged = true;
				} else if ( (*j)->m_nEnd >= nSize ) {
				    (*j)->m_nEnd = nSize - 1;
				    ++j;
				    m_bIsExtendedChanged = true;
				} else {
				    ++j;
				}
			    }
			}
			bHasExtended = true;
			break;
		    }
		    if ( bHasExtended == false ) {
			for ( uint i = 0 ; i < m_pcFirstPTable->m_cLogicalPartitions.size() ; ++i ) {
			    delete m_pcFirstPTable->m_cLogicalPartitions[i];
			}
			m_pcFirstPTable->m_cLogicalPartitions.clear();
			m_bIsExtendedChanged = true;
		    }
		    if ( pcEditView->m_bTypeChanged || pcEditView->m_bLayoutChanged || pcEditView->m_bStatusChanged ) {
			m_bIsPrimaryChanged = true;
		    }
		    
		    if ( m_bLayoutChanged ) {
			(new Alert( MSG_ALERTWND_CHANGEDLAYOUT, MSG_ALERTWND_CHANGEDLAYOUT_TEXT, Alert::ALERT_INFO,0, MSG_ALERTWND_CHANGEDLAYOUT_CANCEL.c_str(), MSG_ALERTWND_CHANGEDLAYOUT_OK.c_str(), NULL ))->Go( new Invoker( new Message( ID_WRITE_PARTITION_TABLE ), this ) );
		    } else if ( m_bTypeChanged ) {
			(new Alert( MSG_ALERTWND_CHANGEDTYPE, MSG_ALERTWND_CHANGEDTYPE_TEXT, Alert::ALERT_INFO,0, MSG_ALERTWND_CHANGEDTYPE_CANCEL.c_str(), MSG_ALERTWND_CHANGEDTYPE_OK.c_str(), NULL ))->Go( new Invoker( new Message( ID_WRITE_PARTITION_TABLE ), this ) );
		    } else {
			(new Alert( MSG_ALERTWND_CHANGEDSTATUS, MSG_ALERTWND_CHANGEDSTATUS_TEXT,Alert::ALERT_INFO, 0, MSG_ALERTWND_CHANGEDSTATUS_CANCEL.c_str(), MSG_ALERTWND_CHANGEDSTATUS_OK.c_str(), NULL ))->Go( new Invoker( new Message( ID_WRITE_PARTITION_TABLE ), this ) );
		    }
		} else {
		    PostMessage( M_QUIT );
		}
	    } else {
		if ( pcEditView->m_bTypeChanged || pcEditView->m_bLayoutChanged || pcEditView->m_bStatusChanged ) {
		    m_bIsExtendedChanged = true;
		}
		m_cEditViews.pop();
		RemoveChild( pcEditView );
		delete pcEditView;
		m_cEditViews.top()->SetFrame( GetBounds() );
		AddChild( m_cEditViews.top() );
	    }
	    break;
	}
	case ID_WRITE_PARTITION_TABLE:
	{
	    int32 nButton = -1;
	    if ( pcMessage->FindInt32( "which", &nButton ) == 0 && nButton == 1 ) {
		std::sort( m_pcFirstPTable->m_cLogicalPartitions.begin(), m_pcFirstPTable->m_cLogicalPartitions.end(), ComparePartitions() );
		WritePartitionTable();
		ioctl( m_cDiskInfo.m_nDiskFD, IOCTL_REREAD_PTABLE );
		PostMessage( M_QUIT );
	    }
	    break;
	}
	default:
	    Window::HandleMessage( pcMessage );
	    break;
    }
}

void PartitionEditReq::WritePartitionTable()
{
    if ( m_bIsPrimaryChanged ) {
	for ( uint i = 0 ; i < m_cPrimary.size() ; ++i ) {
	    if ( m_cPrimary[i]->m_nType == 0 ) {
		memset( &m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i], 0, sizeof(m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i]) );
	    } else {
		uint32 nStart = m_cPrimary[i]->m_nStart * m_cDiskInfo.m_sDriveInfo.sectors_per_track;
		uint32 nEnd   = (m_cPrimary[i]->m_nEnd+1) * m_cDiskInfo.m_sDriveInfo.sectors_per_track - 1;
		uint32 nSize  = (m_cPrimary[i]->m_nEnd - m_cPrimary[i]->m_nStart + 1) * m_cDiskInfo.m_sDriveInfo.sectors_per_track;

		if ( m_cPrimary[i]->IsModified() ) {
		    m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nStart = nStart;
		    m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nSize  = nSize;

		    lba_to_csh( nStart, m_cDiskInfo.m_sDriveInfo,
				&m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nStartCL,
				&m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nStartCH,
				&m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nStartDH );

		    lba_to_csh( nEnd, m_cDiskInfo.m_sDriveInfo,
				&m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nEndCL,
				&m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nEndCH,
				&m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nEndDH );
		
		}
		m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nType   = m_cPrimary[i]->m_nType;
		m_pcFirstPTable->m_sBootBlock.bb_asPartitions[i].p_nStatus = m_cPrimary[i]->m_nStatus;
	    }
	}
	if ( write_pos( m_cDiskInfo.m_nDiskFD, 0, &m_pcFirstPTable->m_sBootBlock, 512 ) != 512 ) {
	}
    }
    if ( m_bIsExtendedChanged ) {
	off_t nUnitSize = m_cDiskInfo.m_sDriveInfo.sectors_per_track * m_cDiskInfo.m_sDriveInfo.bytes_per_sector;
	if ( m_pcFirstPTable->m_cLogicalPartitions.size() == 0 ) {
	    bool bHasExtended = false;
	    for ( uint i = 0 ; i < m_cPrimary.size() ; ++i ) {
		if ( IS_EXTENDED( m_cPrimary[i]->m_nType ) ) {
		    m_nExtendedPos = m_cPrimary[i]->m_nStart;
		    bHasExtended = true;
		    break;
		}
	    }
	    if ( bHasExtended ) {
		BootBlock_s sBootBlock;
		if ( read_pos( m_cDiskInfo.m_nDiskFD, off_t(m_nExtendedPos) * nUnitSize, &sBootBlock, 512 ) == 512 ) {
		    memset( sBootBlock.bb_asPartitions, 0, sizeof(sBootBlock.bb_asPartitions) );

		    sBootBlock.p_nSignature = 0xaa55;

		    if ( write_pos( m_cDiskInfo.m_nDiskFD, off_t(m_nExtendedPos) * nUnitSize, &sBootBlock, 512 ) != 512 ) {
		      // FIXME: ERROR MESSAGE!!!!!!!!
		    }
		} else {
		      // FIXME: ERROR MESSAGE!!!!!!!!
		}
	    }		
	} else {
	    for ( uint i = 0 ; i < m_pcFirstPTable->m_cLogicalPartitions.size() ; ++i ) {
		PartitionEntry* pcEntry = m_pcFirstPTable->m_cLogicalPartitions[i];
		off_t nExtPos = m_nExtendedPos;
		if ( i > 0 ) {
		    nExtPos += pcEntry->m_nStart - 1;
		}
		BootBlock_s sBootBlock;
		if ( read_pos( m_cDiskInfo.m_nDiskFD, nExtPos * nUnitSize, &sBootBlock, 512 ) != 512 ) {
		    break;
		}
		memset( sBootBlock.bb_asPartitions, 0, sizeof(sBootBlock.bb_asPartitions) );

		sBootBlock.p_nSignature = 0xaa55;
	    
		uint32 nSize  = (pcEntry->m_nEnd - pcEntry->m_nStart + 1);
		uint32 nStart = ((i==0) ? pcEntry->m_nStart : 1) * m_cDiskInfo.m_sDriveInfo.sectors_per_track;
		uint32 nEnd   = ((i==0) ? (pcEntry->m_nEnd+1) : (nSize + 1) ) * m_cDiskInfo.m_sDriveInfo.sectors_per_track - 1;
		nSize *= m_cDiskInfo.m_sDriveInfo.sectors_per_track;
	    
		sBootBlock.bb_asPartitions[0].p_nStart = nStart;
		sBootBlock.bb_asPartitions[0].p_nSize  = nSize;
		lba_to_csh( nStart, m_cDiskInfo.m_sDriveInfo,
			    &sBootBlock.bb_asPartitions[0].p_nStartCL,
			    &sBootBlock.bb_asPartitions[0].p_nStartCH,
			    &sBootBlock.bb_asPartitions[0].p_nStartDH );

		lba_to_csh( nEnd, m_cDiskInfo.m_sDriveInfo,
			    &sBootBlock.bb_asPartitions[0].p_nEndCL,
			    &sBootBlock.bb_asPartitions[0].p_nEndCH,
			    &sBootBlock.bb_asPartitions[0].p_nEndDH );
		
	    
		sBootBlock.bb_asPartitions[0].p_nType   = pcEntry->m_nType;
		sBootBlock.bb_asPartitions[0].p_nStatus = 0x00;
		if ( i < m_pcFirstPTable->m_cLogicalPartitions.size() - 1 ) {
		    nSize = m_pcFirstPTable->m_cLogicalPartitions[i+1]->m_nEnd - m_pcFirstPTable->m_cLogicalPartitions[i+1]->m_nStart + 2;
		    off_t nPos = m_pcFirstPTable->m_cLogicalPartitions[i+1]->m_nStart - 1;
		    nStart = (nPos) * m_cDiskInfo.m_sDriveInfo.sectors_per_track;
		    nEnd   = (nPos+nSize) * m_cDiskInfo.m_sDriveInfo.sectors_per_track - 1;

		    sBootBlock.bb_asPartitions[1].p_nStart = nStart;
		    sBootBlock.bb_asPartitions[1].p_nSize  = nSize;
		    lba_to_csh( nStart, m_cDiskInfo.m_sDriveInfo,
				&sBootBlock.bb_asPartitions[1].p_nStartCL,
				&sBootBlock.bb_asPartitions[1].p_nStartCH,
				&sBootBlock.bb_asPartitions[1].p_nStartDH );

		    lba_to_csh( nEnd, m_cDiskInfo.m_sDriveInfo,
				&sBootBlock.bb_asPartitions[1].p_nEndCL,
				&sBootBlock.bb_asPartitions[1].p_nEndCH,
				&sBootBlock.bb_asPartitions[1].p_nEndDH );
		
	    
		    sBootBlock.bb_asPartitions[1].p_nType   = 0x05;
		    sBootBlock.bb_asPartitions[1].p_nStatus = 0x00;
		}
		if ( write_pos( m_cDiskInfo.m_nDiskFD, nExtPos * nUnitSize, &sBootBlock, 512 ) != 512 ) {
		    break;
		}
	    }
	}
    }
}

/******************************************************************************
**** PartitionEdit ************************************************************
******************************************************************************/

PartitionEdit::PartitionEdit( const os::Rect& cFrame, const std::string& cName, DiskInfo cDisk,
			      PartitionList_t* pcPList, off_t nDiskSize, bool bIsExtended ) :
	LayoutView( cFrame, cName )
{
    m_bIsExtended = bIsExtended;
    m_pcPList = pcPList;
    m_nPartitionSize   = nDiskSize;

    m_bLayoutChanged = false;
    m_bTypeChanged   = false;
    m_bStatusChanged = false;
    m_nFirstVisible  = 0;
    
    FrameView* pcLabelFrame = new FrameView( Rect(0,0,0,0), "label_frame", MSG_PARTWND_DISKPARAM );
    VLayoutNode* pcLabelRoot = new VLayoutNode( "label_root" );

    m_sDiskGeom = cDisk.m_sDriveInfo;
    m_cDevicePath = cDisk.m_cPath;
    
    VLayoutNode* pcRoot = new VLayoutNode( "root" );

    char zString[512];

    if ( bIsExtended == false ) {
	sprintf( zString, MSG_PARTWND_DISKPARAM_DISKSIZE.c_str(),
		 double(m_nPartitionSize*m_sDiskGeom.sectors_per_track*m_sDiskGeom.bytes_per_sector) / (1024.0*1024.0) );
    } else {
	sprintf( zString, MSG_PARTWND_DISKPARAM_EXTSIZE.c_str(),
		 double(m_nPartitionSize*m_sDiskGeom.sectors_per_track*m_sDiskGeom.bytes_per_sector) / (1024.0*1024.0) );
    }
    m_pcDiskSizeLabel = new StringView( Rect(), "disk_size", zString );
    
    sprintf( zString, MSG_PARTWND_DISKPARAM_UNITSIZE.c_str(),
	     m_sDiskGeom.sectors_per_track, m_sDiskGeom.bytes_per_sector,
	     m_sDiskGeom.sectors_per_track*m_sDiskGeom.bytes_per_sector );
    m_pcUnitSizeLabel = new StringView( Rect(), "unit_size", zString );
    StringView*	pcUnusedSizeHeader = new StringView( Rect(), "unused_size_label", MSG_PARTWND_DISKPARAM_UNUSESSPACE + " " );
    m_pcUnusedSizeLabel = new StringView( Rect(), "unused_size", "" );

    HLayoutNode* pcLabelCont1 = new HLayoutNode( "label_cont1" );
    HLayoutNode* pcLabelCont2 = new HLayoutNode( "label_cont2" );
    HLayoutNode* pcLabelCont3 = new HLayoutNode( "label_cont3" );

    pcLabelCont1->AddChild( m_pcDiskSizeLabel )->SetBorders( Rect( 10.0f, 5.0f, 10.0f, 0.0f ) );
    pcLabelCont1->AddChild( new HLayoutSpacer( "space" ) );
    pcLabelCont2->AddChild( m_pcUnitSizeLabel )->SetBorders( Rect( 10.0f, 5.0f, 0.0f, 0.0f ) );
    pcLabelCont2->AddChild( new HLayoutSpacer( "space" ) );
    pcLabelCont3->AddChild( pcUnusedSizeHeader )->SetBorders( Rect( 10.0f, 5.0f, 0.0f, 10.0f ) );
    pcLabelCont3->AddChild( m_pcUnusedSizeLabel )->SetBorders( Rect( 0.0f, 5.0f, 10.0f, 5.0f ) );
    pcLabelCont3->AddChild( new HLayoutSpacer( "space" ) );
    
    pcLabelRoot->AddChild( pcLabelCont1 );
    pcLabelRoot->AddChild( pcLabelCont2 );
    pcLabelRoot->AddChild( pcLabelCont3 );
    pcLabelFrame->SetRoot( pcLabelRoot );
    pcRoot->AddChild( pcLabelFrame )->SetBorders( Rect( 15.0f, 20.0f, 15.0f, 10.0f ) );

    for ( uint i = 0 ; i < 4 ; ++i ) {
	char zName[64];


	sprintf( zName, "partition%d", i );
	  /*
	if ( (m_sBootBlock.bb_asPartitions[i].p_nStart % m_sDiskGeom.sectors_per_track) != 0 ) {
	    printf( "Warning: Partition %d have unaligned start address: %ld\n", i, m_sBootBlock.bb_asPartitions[i].p_nStart );
	}
	if ( (m_sBootBlock.bb_asPartitions[i].p_nSize % m_sDiskGeom.sectors_per_track) != 0 ) {
	    printf( "Warning: Partition %d have unaligned size: %ld\n", i, m_sBootBlock.bb_asPartitions[i].p_nSize );
	}*/
	PartitionView* pcView = new PartitionView( this, zName, m_nPartitionSize, m_sDiskGeom.sectors_per_track, m_sDiskGeom.bytes_per_sector );
	pcRoot->AddChild( pcView )->SetBorders( Rect( 15, 5, 15, 5 ) );
	m_cPartitionViews.push_back( pcView );
    }

    m_pcOkBut = new Button( Rect(), "ok_but", (m_bIsExtended) ? MSG_PARTWND_BUTTONS_DONE : MSG_PARTWND_BUTTONS_OK, new Message( ID_OK ) );

    if ( m_bIsExtended == false ) {
	m_pcPartitionBut = new Button( Rect(), "partition_but", MSG_PARTWND_BUTTONS_EDITEXT, new Message( ID_PARTITION_EXTENDED ) );
	m_pcCancelBut = new Button( Rect(), "cancel_but", MSG_PARTWND_BUTTONS_CANCEL, new Message( ID_CANCEL ) );
    } else {
	m_pcPartitionBut = NULL;
	m_pcCancelBut = NULL;
    }
    m_pcClearBut = new Button( Rect(), "clear_but", MSG_PARTWND_BUTTONS_CLEAR, new Message( ID_CLEAR ) );

    if ( m_bIsExtended ) {
	m_pcAddBut       = new Button( Rect(), "add_but", MSG_PARTWND_BUTTONS_ADD, new Message( ID_ADD ) );
	m_pcPrevBut      = new Button( Rect(), "prev_but", "<<", new Message( ID_PREV ) );
	m_pcNextBut      = new Button( Rect(), "next_but", ">>", new Message( ID_NEXT ) );
	m_pcSortBut      = new Button( Rect(), "sort_but", MSG_PARTWND_BUTTONS_SORT, new Message( ID_SORT ) );
    } else {
	m_pcAddBut       = NULL;
	m_pcPrevBut      = NULL;
	m_pcNextBut      = NULL;
	m_pcSortBut	 = NULL;
    }
    HLayoutNode* pcButtonPanel = new HLayoutNode( "button_panel" );

    if ( m_bIsExtended ) {
	pcButtonPanel->AddChild( m_pcPrevBut )->SetBorders( Rect( 10.0f, 10.0f, 5.0f, 10.0f ) );
	pcButtonPanel->AddChild( m_pcNextBut )->SetBorders( Rect( 0.0f, 10.0f, 10.0f, 10.0f ) );
	pcButtonPanel->AddChild( m_pcAddBut )->SetBorders( Rect( 0.0f, 10.0f, 10.0f, 10.0f ) );
    }
    pcButtonPanel->AddChild( new HLayoutSpacer( "space" ) );
    if ( m_bIsExtended == false ) {
	pcButtonPanel->AddChild( m_pcPartitionBut );
    } else {
	pcButtonPanel->AddChild( m_pcSortBut )->SetBorders( Rect( 0.0f, 10.0f, 10.0f, 10.0f ) );
    }
    pcButtonPanel->AddChild( m_pcClearBut );
    pcButtonPanel->AddChild( m_pcOkBut );
    if ( m_bIsExtended == false ) {
	pcButtonPanel->AddChild( m_pcCancelBut );
    }

    pcButtonPanel->SetBorders( Rect( 0.0f, 10.0f, 10.0f, 10.0f ), "ok_but", "partition_but", "cancel_but", "clear_but", NULL );
    pcButtonPanel->SameWidth( "ok_but", "cancel_but", "clear_but", NULL );
    pcButtonPanel->SameWidth( "prev_but", "next_but", NULL );
    
    pcRoot->AddChild( pcButtonPanel );
    
    SetRoot( pcRoot );

    for ( uint i = 0 ; i < m_pcPList->size() && i < 4 ; ++i ) {
	m_cPartitionViews[i]->SetPInfo( (*m_pcPList)[i] );
    }
    EnableNavigationButtons();
}

void PartitionEdit::AllAttached()
{
    m_pcOkBut->SetTarget( this );
    if ( m_pcCancelBut != NULL ) {
	m_pcCancelBut->SetTarget( this );
	m_pcPartitionBut->SetTarget( this );
    }
    m_pcClearBut->SetTarget( this );
    if ( m_pcAddBut != NULL ) {
	m_pcAddBut->SetTarget( this );
    }
    if ( m_pcPrevBut != NULL ) {
	m_pcPrevBut->SetTarget( this );
    }
    if ( m_pcNextBut != NULL ) {
	m_pcNextBut->SetTarget( this );
    }
    if ( m_pcSortBut != NULL ) {
	m_pcSortBut->SetTarget( this );
    }
    MakeFocus();
}

void PartitionEdit::EnableNavigationButtons()
{
    if ( m_bIsExtended ) {
	m_pcPrevBut->SetEnable( m_nFirstVisible > 0 );
	m_pcNextBut->SetEnable( m_pcPList->size() > m_cPartitionViews.size() && int(m_nFirstVisible) < int(m_pcPList->size() - m_cPartitionViews.size()) );
    }
}

void PartitionEdit::SetFirstVisible( uint nFirst )
{
    if ( nFirst != m_nFirstVisible ) {
	m_nFirstVisible = nFirst;

	for ( uint i = 0 ; i < m_cPartitionViews.size() ; ++i ) {
	    m_cPartitionViews[i]->SetPInfo( (*m_pcPList)[m_nFirstVisible+i] );
	}
	
    }
    EnableNavigationButtons();
}

void PartitionEdit::DeletePartition( uint nIndex )
{
    m_bLayoutChanged = true;
    m_pcPList->erase( m_pcPList->begin() + nIndex );
    if ( m_pcPList->size() <= m_cPartitionViews.size() ) {
	m_nFirstVisible = 0;
    } else if ( int(m_nFirstVisible) > int(m_pcPList->size() - m_cPartitionViews.size()) ) {
	m_nFirstVisible = m_pcPList->size() - m_cPartitionViews.size();
    }
    for ( uint i = 0 ; i < m_cPartitionViews.size() ; ++i ) {
	if ( m_nFirstVisible + i < m_pcPList->size() ) {
	    m_cPartitionViews[i]->SetPInfo( (*m_pcPList)[m_nFirstVisible+i] );
	} else {
	    m_cPartitionViews[i]->SetPInfo( NULL );
	}
    }
    EnableNavigationButtons();
    UpdateLogicalDeviceNames();
}


bool PartitionEdit::FindFreeSpace( off_t* pnStart, off_t* pnEnd )
{
    std::vector<PNode> cList;
    int nSpacing = (m_bIsExtended) ? 1 : 0;
    
    for ( uint i = 0 ; i < m_pcPList->size() ; ++i ) {
	if ( (*m_pcPList)[i]->m_nType == 0 ) continue;
	cList.push_back( PNode( (*m_pcPList)[i]->m_nStart, (*m_pcPList)[i]->m_nEnd ) );
    }
    std::sort( cList.begin(), cList.end() );

    if ( cList.size() == 0 ) {
	*pnStart = 1;
	*pnEnd   = m_nPartitionSize - 1;
	return( true );
    } else {
	if ( cList[0].m_nStart > 1 + nSpacing ) {
	    *pnStart = 1;
	    *pnEnd = cList[0].m_nStart - 1 - nSpacing;
	    return( true );
	} else {
	    for ( uint i = 0 ; i < cList.size() ; ++i ) {
		if ( i == cList.size() - 1 ) {
		    if ( cList[i].m_nEnd < m_nPartitionSize - 1 ) {
			*pnStart = cList[i].m_nEnd + 1 + nSpacing;
			*pnEnd   = m_nPartitionSize - 1;
			return( true );
		    }
		} else if ( cList[i].m_nEnd < cList[i+1].m_nStart - 1 - nSpacing ) {
		    *pnStart = cList[i].m_nEnd + 1 + nSpacing;
		    *pnEnd   = cList[i+1].m_nStart - 1 - nSpacing;
		    return( true );
		}
	    }
	}
    }
    return( false );
}

void PartitionEdit::UpdateLogicalDeviceNames()
{
    PartitionList_t cList = *m_pcPList;

    std::sort( cList.begin(), cList.end(), ComparePartitions() );

    std::string cDevicePath = std::string( m_cDevicePath.begin(), m_cDevicePath.end() - 3 );
    
    for ( uint i = 0 ; i < cList.size() ; ++i ) {
	char zName[16];
	sprintf( zName, "%d", 4 + i );
	cList[i]->m_cPartitionPath = cDevicePath + zName;
    }
    for ( uint i = 0 ; i < m_cPartitionViews.size() && i < m_pcPList->size() ; ++i ) {
	m_cPartitionViews[i]->SetPath( (*m_pcPList)[m_nFirstVisible+i]->m_cPartitionPath );
    }
    
}

void PartitionEdit::CreatePartition( int nType )
{
    off_t nStart;
    off_t nEnd;
    
    if ( FindFreeSpace( &nStart, &nEnd ) == false ) {
	return;
    }

    m_bLayoutChanged = true;
    
    PartitionEntry* pcEntry = new PartitionEntry;
    pcEntry->m_nType   = nType;
    pcEntry->m_nStatus = 0x00;
    pcEntry->m_nStart  = nStart;
    pcEntry->m_nEnd    = nEnd;

    pcEntry->m_nOrigStart  = pcEntry->m_nStart;
    pcEntry->m_nOrigEnd    = pcEntry->m_nEnd;
    pcEntry->m_nOrigType   = pcEntry->m_nType;
    pcEntry->m_nOrigStatus = pcEntry->m_nStatus;
    
    pcEntry->m_bIsMounted     = false;
    pcEntry->m_cVolumeName    = MSG_PARTWND_PARTINFO_UNKNOWN;
    m_pcPList->push_back( pcEntry );

    if ( m_pcPList->size() > m_cPartitionViews.size() ) {
	m_nFirstVisible = m_pcPList->size() - m_cPartitionViews.size();
    }
    
    for ( uint i = 0 ; i < m_cPartitionViews.size() ; ++i ) {
	if ( m_nFirstVisible + i < m_pcPList->size() ) {
	    m_cPartitionViews[i]->SetPInfo( (*m_pcPList)[m_nFirstVisible+i] );
	} else {
	    m_cPartitionViews[i]->SetPInfo( NULL );
	}
    }
    UpdateLogicalDeviceNames();
    EnableNavigationButtons();
}

void PartitionEdit::PartitionTypeChanged()
{
    uint nNumExtended = 0;
    uint nNumUnused   = 0;
    for ( uint i = 0 ; i < m_pcPList->size() ; ++i ) {
	int nType = (*m_pcPList)[i]->m_nType;
	if ( IS_EXTENDED( nType ) ) {
	    nNumExtended++;
	}
	if ( nType == 0x00 ) {
	    nNumUnused++;
	}
    }
    if ( m_pcPartitionBut != NULL ) {
	m_pcPartitionBut->SetEnable( nNumExtended == 1 );
    }
    m_pcClearBut->SetEnable( nNumUnused != m_pcPList->size() );
}

void PartitionEdit::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_OK:
	{
	    for ( uint i = 0 ; i < m_pcPList->size() ; ++i ) {
		if ( (*m_pcPList)[i]->m_nType != (*m_pcPList)[i]->m_nOrigType ) {
		    m_bTypeChanged = true;
		    if ( (*m_pcPList)[i]->m_nType == 0 ) {
			m_bLayoutChanged = true;
			break;
		    }
		}
		if ( (*m_pcPList)[i]->m_nStatus != (*m_pcPList)[i]->m_nOrigStatus ) {
		    m_bStatusChanged = true;
		}
		if ( (*m_pcPList)[i]->m_nType != 0 && (*m_pcPList)[i]->m_nOrigType != 0  ) {
		    if ( (*m_pcPList)[i]->IsModified() ) {
			m_bLayoutChanged = true;
			break;
		    }
		}
	    }
	    Message cMsg( ID_END_EXTENDED_EDIT );
	    cMsg.AddBool( "do_save", true );
	    GetLooper()->PostMessage( &cMsg, GetLooper() );
	    break;
	}
	case ID_CANCEL:
	{
	    Message cMsg( ID_END_EXTENDED_EDIT );
	    cMsg.AddBool( "do_save", false );
	    GetLooper()->PostMessage( &cMsg, GetLooper() );
	    break;
	}
	case ID_CLEAR:
	    for ( uint i = 0 ; i < m_cPartitionViews.size() ; ++i ) {
		m_cPartitionViews[i]->SetPartitionType( 0, false );
	    }
	    for ( uint i = 0 ; i < m_pcPList->size() ; ++i ) {
		(*m_pcPList)[i]->m_nType = 0;
	    }
	    break;
	case ID_PARTITION_EXTENDED:
	    for ( uint i = 0 ; i < m_pcPList->size() ; ++i ) {
		if ( IS_EXTENDED( m_cPartitionViews[i]->GetType() ) ) {
		    Message cMsg( ID_PARTITION_EXTENDED );
		    cMsg.AddInt32( "pindex", i );
		    cMsg.AddInt64( "offset", m_cPartitionViews[i]->GetStart() );
		    cMsg.AddInt64( "size", m_cPartitionViews[i]->GetEnd() - m_cPartitionViews[i]->GetStart() + 1 );
		    GetLooper()->PostMessage( &cMsg, GetLooper() );
		    break;
		}
	    }
	    break;
	case ID_SORT:
	    std::sort( m_pcPList->begin(), m_pcPList->end(), ComparePartitions() );

	    for ( uint i = 0 ; i < m_cPartitionViews.size() ; ++i ) {
		if ( i < m_pcPList->size() ) {
		    m_cPartitionViews[i]->SetPInfo( (*m_pcPList)[m_nFirstVisible+i] );
		} else {
		    m_cPartitionViews[i]->SetPInfo( NULL );
		}
	    }
	    break;
	case ID_ADD:
	    CreatePartition( 0x2a );
	    break;
	case ID_PREV:
	    SetFirstVisible( m_nFirstVisible - 1 );
	    break;
	case ID_NEXT:
	    SetFirstVisible( m_nFirstVisible + 1 );
	    break;
	default:
	    LayoutView::HandleMessage( pcMessage );
	    break;
    }
}

void PartitionEdit::MouseDown( const Point& cPosition, uint32 nButton )
{
    MakeFocus();
}

void PartitionEdit::KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers )
{
    switch( pzString[0] ) {
	case VK_UP_ARROW:
	    if ( m_nFirstVisible > 0 ) {
		SetFirstVisible( m_nFirstVisible - 1 );
	    }
	    break;
	case VK_DOWN_ARROW:
	    if ( m_nFirstVisible < m_pcPList->size() - m_cPartitionViews.size() ) {
		SetFirstVisible( m_nFirstVisible + 1 );
	    }
	    break;
    }
    
}

void PartitionEdit::UpdateFreeSpace()
{
    off_t nUnitSize = m_sDiskGeom.sectors_per_track*m_sDiskGeom.bytes_per_sector;
    
    off_t nSize = nUnitSize;
    for ( uint i = 0 ; i < m_pcPList->size() ; ++i ) {
	if ( (*m_pcPList)[i]->m_nType != 0 ) {
	    nSize += (*m_pcPList)[i]->GetByteSize(m_sDiskGeom);
	    if ( i != 0 && m_bIsExtended ) {
		nSize += nUnitSize;
	    }
	}
    }
    if ( m_bIsExtended ) {
	m_pcAddBut->SetEnable( nSize < m_nPartitionSize*nUnitSize );
	for ( uint i = 0 ; i < m_cPartitionViews.size() ; ++i ) {
	    m_cPartitionViews[i]->EnableTypeMenu( m_nFirstVisible + i < m_pcPList->size() );
	}
    } else {
	for ( uint i = 0 ; i < m_cPartitionViews.size() ; ++i ) {
	    m_cPartitionViews[i]->EnableTypeMenu( m_cPartitionViews[i]->GetType() != 0 || nSize < m_nPartitionSize*nUnitSize );
	}
    }
    char zString[256];
    sprintf( zString, MSG_PARTWND_PARTINFO_PARTSIZE.c_str(), double(m_nPartitionSize*nUnitSize - nSize) / (1024.0*1024.0) );
    m_pcUnusedSizeLabel->SetString( zString );
      // FIXME: Remove this when auto-relayout is implemented in the layout kit.
    ((LayoutView*)m_pcUnusedSizeLabel->GetParent())->FindNode( "label_cont3" )->Layout();
}

/******************************************************************************
**** PartitionView ************************************************************
******************************************************************************/

PartitionView::PartitionView( PartitionEdit* pcParent, const std::string& cName,
			      off_t nDiskSize, int nCylSize, int nSectorSize ) :
	FrameView( Rect(), cName, MSG_PARTWND_PARTINFO_UNUSED, CF_FOLLOW_NONE )
{
    m_pcPInfo = NULL;

    m_pcParent      = pcParent;
    m_nDiskSize     = nDiskSize;
    m_nCylinderSize = nCylSize;
    m_nSectorSize   = nSectorSize;

    if ( m_nCylinderSize < 1 ) {
	m_nCylinderSize = 63;
    }
    m_pcBarView	     = new PartitionBar( Rect(), this, "partition_bar", 0.0, 0.0 );
    if ( m_pcParent->m_bIsExtended == false ) {
	m_pcActiveCB = new CheckBox( Rect(), "status_cb", "", new Message( ID_PARTITION_STATUS ) );
    } else {
	m_pcActiveCB = NULL;
    }
    m_pcStartSpinner = new Spinner( Rect(), "start", 0.0, new Message( ID_START_CHANGED ) );
    m_pcEndSpinner   = new Spinner( Rect(), "end", 0.0, new Message( ID_END_CHANGED ) );
    m_pcTypeMenu     = new DropdownMenu( Rect(), "type" );
    m_pcVolumeName   = new StringView( Rect(), "volume_label", "" );
    m_pcPartitionSize= new StringView( Rect(), "partition_size", "", ALIGN_RIGHT );

    for ( int i = 0 ; g_asPartitionTypes[i].pt_pzName != NULL ; ++i ) {
	String cType = g_asPartitionTypes[i].pt_pzName;

	if( strcmp( g_asPartitionTypes[i].pt_pzSystem, "" ) != 0 )
		cType.Format( "%s - %s", g_asPartitionTypes[i].pt_pzName, g_asPartitionTypes[i].pt_pzSystem );
	else
		cType.Format( "%s", g_asPartitionTypes[i].pt_pzName );

	m_pcTypeMenu->AppendItem( cType );
    }
    

    m_pcStartSpinner->SetEnable( false );
    m_pcEndSpinner->SetEnable( false );
    m_pcTypeMenu->SetEnable( false );
    
    m_pcStartSpinner->SetMinPreferredSize( 11 );
    m_pcStartSpinner->SetMaxPreferredSize( 11 );
    m_pcStartSpinner->SetFormat( "%.0f" );
    m_pcStartSpinner->SetScale( 10*1024*1024 / (m_nCylinderSize * m_nSectorSize) );
    m_pcStartSpinner->SetStep( 1.0f );
    
    m_pcEndSpinner->SetMinPreferredSize( 11 );
    m_pcEndSpinner->SetMaxPreferredSize( 11 );
    m_pcEndSpinner->SetFormat( "%.0f" );
    m_pcEndSpinner->SetScale( 10*1024*1024 / (m_nCylinderSize * m_nSectorSize) );
    m_pcEndSpinner->SetStep( 1.0f );

    m_pcPartitionSize->SetMinPreferredSize( 15 );
    m_pcPartitionSize->SetMaxPreferredSize( 15 );

    m_pcTypeMenu->SetSelectionMessage( new Message( ID_TYPE_SELECTED ) );
    m_pcTypeMenu->SetEditMessage( new Message( ID_TYPE_EDITED ) );
    
    VLayoutNode* pcRoot   = new VLayoutNode( "root" );
    HLayoutNode* pcTop    = new HLayoutNode( "top" );
    HLayoutNode* pcMiddle = new HLayoutNode( "middle" );
    HLayoutNode* pcBottom = new HLayoutNode( "bottom" );

    pcTop->SetBorders( Rect( 0.0f, 5.0f, 0.0f, 2.0f ) );
    
    pcRoot->AddChild( pcBottom );
    pcRoot->AddChild( pcTop );
    pcRoot->AddChild( pcMiddle );

    pcTop->AddChild( m_pcVolumeName )->SetBorders( Rect( 10.0f, 0.0f, 0.0f, 0.0f ) );
    pcTop->AddChild( new HLayoutSpacer( "space" ) );
    pcTop->AddChild( m_pcPartitionSize )->SetBorders( Rect( 10.0f, 0.0f, 10.0f, 0.0f ) );
    
    pcMiddle->AddChild( m_pcBarView );
    
    pcBottom->AddChild( m_pcStartSpinner );
    if ( m_pcActiveCB != NULL ) {
	pcBottom->AddChild( m_pcActiveCB )->SetBorders( Rect( 5.0f, 0.0f, 5.0f, 0.0f ) );
    }
    pcBottom->AddChild( new HLayoutSpacer( "space" ) );
    pcBottom->AddChild( m_pcTypeMenu )->SetBorders( Rect( 10.0f, 0.0f, 10.0f, 0.0f ) );;
    pcBottom->AddChild( m_pcEndSpinner );

    SetRoot( pcRoot );
}

uint PartitionView::GetIndex() const
{
    uint nIndex = std::find( m_pcParent->m_cPartitionViews.begin(), m_pcParent->m_cPartitionViews.end(), this ) - m_pcParent->m_cPartitionViews.begin();
    nIndex += m_pcParent->m_nFirstVisible;
    return( nIndex );
}

void PartitionView::SetPInfo( PartitionEntry* pcPInfo )
{
    m_pcPInfo = pcPInfo;
    if ( m_pcPInfo != NULL ) {
	SetLabel( m_pcPInfo->m_cPartitionPath );
	if ( m_pcActiveCB != NULL ) {
	    m_pcActiveCB->SetValue( (m_pcPInfo->m_nStatus & 0x80) != 0 );
	}

	m_pcStartSpinner->SetValue( (m_pcPInfo->m_nType==0) ? 0.0 : double(m_pcPInfo->m_nStart), false );
	m_pcEndSpinner->SetValue( (m_pcPInfo->m_nType==0) ? 0.0 : double(m_pcPInfo->m_nEnd), false );

	m_pcBarView->SetStart( double(m_pcPInfo->m_nStart) / double(m_nDiskSize-1) );
	m_pcBarView->SetEnd( double(m_pcPInfo->m_nEnd) / double(m_nDiskSize-1) );

	m_pcVolumeName->SetString( m_pcPInfo->m_cVolumeName.c_str() );
	
	m_pcStartSpinner->SetEnable( m_pcPInfo->m_nType != 0 && m_pcPInfo->m_bIsMounted == false );
	m_pcEndSpinner->SetEnable( m_pcPInfo->m_nType != 0 && m_pcPInfo->m_bIsMounted == false );

	bool bNumerical = true;
	for ( int i = 0 ; g_asPartitionTypes[i].pt_pzName != NULL ; ++i ) {
	    if ( g_asPartitionTypes[i].pt_nType == m_pcPInfo->m_nType ) {
		m_pcBarView->SetColor( g_asPartitionTypes[i].pt_nColor );
		m_pcTypeMenu->SetSelection( i, false );
		bNumerical = false;
	    }
	}
	if ( bNumerical ) {
	    char zString[64];
	    sprintf( zString, "0x%02x", m_pcPInfo->m_nType );
	    m_pcTypeMenu->SetCurrentString( zString );
	}
	SetSizeString();
	SetSpinnerLimits();
    } else {
	SetLabel( MSG_PARTWND_PARTINFO_UNUSED );
	m_pcTypeMenu->SetSelection( 0, false );
	m_pcStartSpinner->SetEnable( false );
	m_pcEndSpinner->SetEnable( false );
	
	m_pcStartSpinner->SetValue( 0.0, false );
	m_pcEndSpinner->SetValue( 0.0, false );

	m_pcTypeMenu->SetEnable( false );
	
	m_pcVolumeName->SetString( MSG_PARTWND_PARTINFO_UNKNOWN );
	SetSizeString();
	m_pcBarView->Invalidate();
    }
      // FIXME: Remove this when auto-relayout is implemented in the layout kit.
    ((LayoutView*)m_pcVolumeName->GetParent())->FindNode( "root" )->Layout();
}


void PartitionView::AttachedToWindow()
{
    m_pcStartSpinner->SetTarget( this );
    m_pcEndSpinner->SetTarget( this );
    m_pcTypeMenu->SetTarget( this );
    if ( m_pcActiveCB != NULL ) {
	m_pcActiveCB->SetTarget( this );
    }
}

void PartitionView::EnableTypeMenu( bool bEnable )
{
    if ( m_pcPInfo != NULL ) {
	m_pcTypeMenu->SetEnable( bEnable );
    }
}

void PartitionView::SetSizeString()
{
    if ( m_pcPInfo != NULL && m_pcPInfo->m_nType != 0 ) {
	char zString[128];
	sprintf( zString, MSG_PARTWND_PARTINFO_PARTSIZE.c_str(), double((m_pcPInfo->m_nEnd - m_pcPInfo->m_nStart + 1) * m_nCylinderSize * m_nSectorSize) / (1024.0*1024.0) );
	m_pcPartitionSize->SetString( zString );
    } else {
	m_pcPartitionSize->SetString( MSG_PARTWND_PARTINFO_PARTSIZEZERO );
    }
}

off_t PartitionView::GetByteStart() const
{
    if ( m_pcPInfo != NULL && m_pcPInfo->m_nType != 0 ) {
	return( m_pcPInfo->m_nStart * m_pcParent->m_sDiskGeom.sectors_per_track * m_pcParent->m_sDiskGeom.bytes_per_sector );
    } else {
	return( 0 );
    }
}

off_t PartitionView::GetByteSize() const
{
    if ( m_pcPInfo != NULL && m_pcPInfo->m_nType != 0 ) {
	return( (m_pcPInfo->m_nEnd - m_pcPInfo->m_nStart + 1) * m_pcParent->m_sDiskGeom.sectors_per_track * m_pcParent->m_sDiskGeom.bytes_per_sector );
    } else {
	return( 0 );
    }
}

bool PartitionView::IsValidExtended() const
{/*
    if ( m_pcParent->m_pcPTable->m_pcExtended == NULL || IS_EXTENDED( m_pcPInfo->m_nType ) == false ) {
	return( false );
    }
    return( GetByteStart() == m_pcParent->m_pcPTable->m_pcExtended->m_nPosition &&
	    GetByteSize() == m_pcParent->m_pcPTable->m_pcExtended->m_nSize );*/
    return( false );
    
}

void PartitionView::SetSpinnerLimits()
{
    int nSpacing = (m_pcParent->IsExtended()) ? 1 : 0;
    
    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
	(*m_pcParent->m_pcPList)[i]->m_nMin = 63;
	(*m_pcParent->m_pcPList)[i]->m_nMax = m_nDiskSize - 1;
    }
    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
	for ( uint j = 0 ; j < m_pcParent->m_pcPList->size() ; ++j ) {
	    if ( i == j  || (*m_pcParent->m_pcPList)[i]->m_nType == 0 ) continue;
	    if ( (*m_pcParent->m_pcPList)[j]->m_nStart < (*m_pcParent->m_pcPList)[i]->m_nStart ) {
		if ( (*m_pcParent->m_pcPList)[j]->m_nMax >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
		    (*m_pcParent->m_pcPList)[j]->m_nMax = (*m_pcParent->m_pcPList)[i]->m_nStart - 1 - nSpacing;
		}
	    } else {
		if ( (*m_pcParent->m_pcPList)[j]->m_nMin <= (*m_pcParent->m_pcPList)[i]->m_nEnd + nSpacing ) {
		{
		    (*m_pcParent->m_pcPList)[j]->m_nMin = (*m_pcParent->m_pcPList)[i]->m_nEnd + 1 + nSpacing;
			if( (*m_pcParent->m_pcPList)[j]->m_nMin < 63 )
				(*m_pcParent->m_pcPList)[j]->m_nMin = 63;
		}

		}
	    }
	}
    }
    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
	if ( (*m_pcParent->m_pcPList)[i]->m_nMin > (*m_pcParent->m_pcPList)[i]->m_nMax ) {
	    (*m_pcParent->m_pcPList)[i]->m_nMin = (*m_pcParent->m_pcPList)[i]->m_nMax;
	}
	if ( (*m_pcParent->m_pcPList)[i]->m_nMax < (*m_pcParent->m_pcPList)[i]->m_nMin ) {
	    (*m_pcParent->m_pcPList)[i]->m_nMax = (*m_pcParent->m_pcPList)[i]->m_nMin;
	}
	if ( i >= m_pcParent->m_nFirstVisible && i < m_pcParent->m_nFirstVisible + m_pcParent->m_cPartitionViews.size() ) {
	    m_pcParent->m_cPartitionViews[i-m_pcParent->m_nFirstVisible]->m_pcStartSpinner->SetMinValue( (*m_pcParent->m_pcPList)[i]->m_nMin );
	    m_pcParent->m_cPartitionViews[i-m_pcParent->m_nFirstVisible]->m_pcStartSpinner->SetMaxValue( (*m_pcParent->m_pcPList)[i]->m_nEnd );
	    m_pcParent->m_cPartitionViews[i-m_pcParent->m_nFirstVisible]->m_pcEndSpinner->SetMinValue( (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing );
	    m_pcParent->m_cPartitionViews[i-m_pcParent->m_nFirstVisible]->m_pcEndSpinner->SetMaxValue( (*m_pcParent->m_pcPList)[i]->m_nMax );
	}
    }
    SetSizeString();
    m_pcParent->UpdateFreeSpace();
}

void PartitionView::SliderStartChanged( float* pvStart )
{
    assert( m_pcPInfo != NULL );

    int nSpacing = (m_pcParent->IsExtended()) ? 1 : 0;
    
    m_pcPInfo->m_nStart = off_t( double(m_nDiskSize-1) * (*pvStart) );

    if ( m_pcPInfo->m_nStart < 63 ) {
	m_pcPInfo->m_nStart = 63;
    }
    uint nIndex = GetIndex();

    bool bUpdateNames = false;
    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
	if ( i == nIndex || (*m_pcParent->m_pcPList)[i]->m_nType == 0 ) continue;
	
	if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd && m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
	    if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd ) {
		if ( m_pcPInfo->m_nStart < (*m_pcParent->m_pcPList)[i]->m_nStart ) {
		    off_t nNewEnd = (*m_pcParent->m_pcPList)[i]->m_nStart - 1 - nSpacing;
		    bool  bCollide = false;
		    for ( uint j = 0 ; j < m_pcParent->m_pcPList->size() ; ++j ) {
			if ( j == nIndex || j == i  || (*m_pcParent->m_pcPList)[j]->m_nType == 0 ) continue;
			if ( nNewEnd >= (*m_pcParent->m_pcPList)[j]->m_nStart - nSpacing &&
			     nNewEnd <= (*m_pcParent->m_pcPList)[j]->m_nEnd ) {
			    bCollide = true;
			    break;
			}
		    }
		    if ( bCollide  ) {
			continue;
		    }
		    m_pcPInfo->m_nEnd = nNewEnd;
		    m_pcBarView->SetEnd( double(m_pcPInfo->m_nEnd) / double(m_nDiskSize-1) );
		    m_pcEndSpinner->SetValue( double(m_pcPInfo->m_nEnd), false );
		    bUpdateNames = true;
		    break;
		}
	    }
	}
    }

    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
	if ( i == nIndex || (*m_pcParent->m_pcPList)[i]->m_nType == 0 ) continue;
	if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd &&
	     m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
	    if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd ) {
		m_pcPInfo->m_nStart = (*m_pcParent->m_pcPList)[i]->m_nEnd + 1 + nSpacing;
	    }
	}
    }
    
    *pvStart = double(m_pcPInfo->m_nStart) / double(m_nDiskSize-1);
    m_pcStartSpinner->SetValue( double(m_pcPInfo->m_nStart), false );
    SetSpinnerLimits();
    if ( bUpdateNames ) {
	m_pcParent->UpdateLogicalDeviceNames();
    }
}

void PartitionView::SliderEndChanged( float* pvEnd )
{
    assert( m_pcPInfo != NULL );

    int nSpacing = (m_pcParent->IsExtended()) ? 1 : 0;
    
    m_pcPInfo->m_nEnd   = off_t( double(m_nDiskSize-1) * (*pvEnd) );
    
    if ( m_pcPInfo->m_nEnd < m_pcPInfo->m_nStart - nSpacing ) {
	m_pcPInfo->m_nEnd = m_pcPInfo->m_nStart - nSpacing;
    }
    uint nIndex = std::find( m_pcParent->m_cPartitionViews.begin(), m_pcParent->m_cPartitionViews.end(), this ) - m_pcParent->m_cPartitionViews.begin();
    nIndex += m_pcParent->m_nFirstVisible;

    bool bUpdateNames = false;
    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
	if ( i == nIndex || (*m_pcParent->m_pcPList)[i]->m_nType == 0 ) continue;
	if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd &&
	     m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
	    if ( m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
		if ( m_pcPInfo->m_nEnd > (*m_pcParent->m_pcPList)[i]->m_nEnd ) {
		    off_t nNewStart = (*m_pcParent->m_pcPList)[i]->m_nEnd + 1;
		    bool  bCollide = false;
		    for ( uint j = 0 ; j < m_pcParent->m_pcPList->size() ; ++j ) {
			if ( j == nIndex || j == i || (*m_pcParent->m_pcPList)[j]->m_nType == 0 ) continue;
			if ( nNewStart >= (*m_pcParent->m_pcPList)[j]->m_nStart - nSpacing &&
			     nNewStart <= (*m_pcParent->m_pcPList)[j]->m_nEnd ) {
			    bCollide = true;
			    break;
			}
		    }
		    if ( bCollide ) {
			continue;
		    }
		    m_pcPInfo->m_nStart = nNewStart + nSpacing;
		    m_pcBarView->SetStart( double(m_pcPInfo->m_nStart) / double(m_nDiskSize-1) );
		    m_pcStartSpinner->SetValue( double(m_pcPInfo->m_nStart), false );
		    bUpdateNames = true;
		    break;
		}
	    }
	}
    }
    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
	if ( i == nIndex || (*m_pcParent->m_pcPList)[i]->m_nType == 0 ) continue;
	if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd &&
	     m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
	    if ( m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
		m_pcPInfo->m_nEnd = (*m_pcParent->m_pcPList)[i]->m_nStart - 1 - nSpacing;
	    }
	}
    }
    
    *pvEnd = double(m_pcPInfo->m_nEnd) / double(m_nDiskSize-1);
    m_pcEndSpinner->SetValue( double(m_pcPInfo->m_nEnd), false );
    SetSpinnerLimits();
    if ( bUpdateNames ) {
	m_pcParent->UpdateLogicalDeviceNames();
    }
}

bool PartitionView::FindFreeSpace()
{
    assert( m_pcPInfo != NULL );
    std::vector<PNode> cList;

    uint nIndex = GetIndex();
    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
	if ( i == nIndex || (*m_pcParent->m_pcPList)[i]->m_nType == 0 ) continue;
	cList.push_back( PNode( (*m_pcParent->m_pcPList)[i]->m_nStart, (*m_pcParent->m_pcPList)[i]->m_nEnd ) );
    }
    std::sort( cList.begin(), cList.end() );

    if ( cList.size() == 0 ) {
	m_pcPInfo->m_nStart = 63;
	m_pcPInfo->m_nEnd = m_nDiskSize - 1;
	return( true );
    } else {
	if ( cList[0].m_nStart > 1 ) {
	    m_pcPInfo->m_nStart = 63;
	    m_pcPInfo->m_nEnd = cList[0].m_nStart - 1;
	    return( true );
	} else {
	    for ( uint i = 0 ; i < cList.size() ; ++i ) {
		if ( i == cList.size() - 1 ) {
		    if ( cList[i].m_nEnd < m_nDiskSize - 1 ) {
			m_pcPInfo->m_nStart = cList[i].m_nEnd + 1;
			m_pcPInfo->m_nEnd   = m_nDiskSize - 1;
			return( true );
		    }
		} else if ( cList[i].m_nEnd < cList[i+1].m_nStart - 1 ) {
		    m_pcPInfo->m_nStart = cList[i].m_nEnd + 1;
		    m_pcPInfo->m_nEnd   = cList[i+1].m_nStart - 1;
		    return( true );
		}
	    }
	}
    }
    return( false );
}

void PartitionView::SetPath( const std::string& cPath )
{
    SetLabel( cPath );
}

void PartitionView::SetPartitionType( int nType, bool bPromptDelete )
{
    if ( m_pcPInfo == NULL || nType == m_pcPInfo->m_nType ) {
	return;
    }
    if ( m_pcPInfo->m_nType == 0 ) {
	if ( FindFreeSpace() ) {
	    m_pcBarView->SetStart( double(m_pcPInfo->m_nStart) / double(m_nDiskSize-1) );
	    m_pcBarView->SetEnd( double(m_pcPInfo->m_nEnd) / double(m_nDiskSize-1) );
	    m_pcStartSpinner->SetValue( m_pcPInfo->m_nStart );
	    m_pcEndSpinner->SetValue( m_pcPInfo->m_nEnd );
	    m_pcPInfo->m_nType = nType;
	}
    } else {
	if ( nType == 0 && bPromptDelete ) {
	    if ( m_pcPInfo->m_bIsMounted == false ) {
		(new Alert( MSG_ALERTWND_DELPART, MSG_ALERTWND_DELPART_TEXT, Alert::ALERT_INFO,0, MSG_ALERTWND_DELPART_CANCEL.c_str(), MSG_ALERTWND_DELPART_OK.c_str(), NULL ))->Go( new Invoker( new Message( ID_DELETE_PARTITION ), this ) );
	    }
	} else {
	    if ( nType == 0 && m_pcParent->IsExtended() ) {
		m_pcParent->DeletePartition( GetIndex() );
		return;
	    } else {
		m_pcPInfo->m_nType = nType;
	    }
	}
    }
    bool bNumerical = true;
    for ( int i = 0 ; g_asPartitionTypes[i].pt_pzName != NULL ; ++i ) {
	if ( g_asPartitionTypes[i].pt_nType == m_pcPInfo->m_nType ) {
	    m_pcBarView->SetColor( g_asPartitionTypes[i].pt_nColor );
	    m_pcTypeMenu->SetSelection( i, false );
	    bNumerical = false;
	    break;
	}
    }
    if ( bNumerical ) {
	char zString[64];
	sprintf( zString, "0x%02x", m_pcPInfo->m_nType );
	m_pcTypeMenu->SetCurrentString( zString );
    }
    m_pcBarView->Invalidate();
    m_pcStartSpinner->SetEnable( m_pcPInfo->m_nType != 0 && m_pcPInfo->m_bIsMounted == false );
    m_pcEndSpinner->SetEnable( m_pcPInfo->m_nType != 0 && m_pcPInfo->m_bIsMounted == false );
    if ( m_pcPInfo->m_nType == 0 ) {
	m_pcPInfo->m_nStart = 0;
	m_pcPInfo->m_nEnd   = 0;
	m_pcStartSpinner->SetValue( 0.0 );
	m_pcEndSpinner->SetValue( 0.0 );
    }
    Flush();
    m_pcParent->PartitionTypeChanged();
}

void PartitionView::HandleMessage( os::Message* pcMessage )
{
    switch( pcMessage->GetCode() )
    {
	case ID_START_CHANGED:
	{
	    if ( m_pcPInfo == NULL ) {
		break;
	    }
	    int nSpacing = (m_pcParent->IsExtended()) ? 1 : 0;
	    off_t nValue = (off_t)floor(m_pcStartSpinner->GetValue());
	    m_pcPInfo->m_nStart = nValue;

	    uint nIndex = std::find( m_pcParent->m_cPartitionViews.begin(), m_pcParent->m_cPartitionViews.end(), this ) - m_pcParent->m_cPartitionViews.begin();
	    nIndex += m_pcParent->m_nFirstVisible;
	    
	    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
		if ( i == nIndex || (*m_pcParent->m_pcPList)[i]->m_nType == 0 ) continue;
		if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd &&
		     m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
		    if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd ) {
			m_pcPInfo->m_nStart = (*m_pcParent->m_pcPList)[i]->m_nEnd + 1 + nSpacing;
		    }
		}
	    }
	    m_pcBarView->SetStart( double(m_pcPInfo->m_nStart) / double(m_nDiskSize-1) );
	    SetSpinnerLimits();
	    break;
	}
	case ID_END_CHANGED:
	{
	    if ( m_pcPInfo == NULL ) {
		break;
	    }
	    int nSpacing = (m_pcParent->IsExtended()) ? 1 : 0;
	    off_t nValue = (off_t)floor(m_pcEndSpinner->GetValue());
	    m_pcPInfo->m_nEnd = nValue;
	    uint nIndex = std::find( m_pcParent->m_cPartitionViews.begin(), m_pcParent->m_cPartitionViews.end(), this ) - m_pcParent->m_cPartitionViews.begin();
	    nIndex += m_pcParent->m_nFirstVisible;
	    for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
		if ( i == nIndex || (*m_pcParent->m_pcPList)[i]->m_nType == 0 ) continue;
		if ( m_pcPInfo->m_nStart - nSpacing <= (*m_pcParent->m_pcPList)[i]->m_nEnd &&
		     m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
		    if ( m_pcPInfo->m_nEnd >= (*m_pcParent->m_pcPList)[i]->m_nStart - nSpacing ) {
			m_pcPInfo->m_nEnd = (*m_pcParent->m_pcPList)[i]->m_nStart - 1 - nSpacing;
		    }
		}
	    }
	    m_pcBarView->SetEnd( double(m_pcPInfo->m_nEnd) / double(m_nDiskSize-1) );
	    SetSpinnerLimits();
	    break;
	}
	case ID_TYPE_SELECTED:
	{
	    if ( m_pcPInfo == NULL )
			break;

	    int nSelection = m_pcTypeMenu->GetSelection();
	    if ( nSelection >= 0 )
			SetPartitionType( g_asPartitionTypes[nSelection].pt_nType, true );

	    break;
	}
	case ID_TYPE_EDITED:
	{
	    if ( m_pcPInfo == NULL ) {
		break;
	    }
	    const std::string& cString = m_pcTypeMenu->GetCurrentString();
	    bool bNumerical = true;
	    int	 nType = m_pcPInfo->m_nType;
	    
	    if ( cString.size() > 2 && cString[0] == '0' && cString[1] == 'x' ) {
		for ( uint i = 2 ; i < cString.size() ; ++i ) {
		    if ( cString[i] < '0' || cString[i] > '9' ) {
			bNumerical = false;
			break;
		    }
		}
		if ( bNumerical ) {
		    sscanf( cString.c_str() + 2, "%x", &nType );
		}
	    } else {
		for ( uint i = 0 ; i < cString.size() ; ++i ) {
		    if ( cString[i] < '0' || cString[i] > '9' ) {
			bNumerical = false;
			break;
		    }
		}
		if ( bNumerical ) {
		    sscanf( cString.c_str(), "%d", &nType );
		}
	    }
	    
	    if ( bNumerical == false ) {
		for ( int i = 0 ; g_asPartitionTypes[i].pt_pzName != NULL ; ++i ) {
		    if ( strcasecmp( cString.c_str(), g_asPartitionTypes[i].pt_pzName ) == 0 ) {
			nType = g_asPartitionTypes[i].pt_nType;
			break;
		    }
       		}
	    }
	    SetPartitionType( nType, true );
	    break;
	}
	case ID_PARTITION_STATUS:
	    if ( m_pcPInfo == NULL ) {
		break;
	    }
	    assert( m_pcActiveCB != NULL );
	    if ( m_pcActiveCB->GetValue().AsBool() != false ) {
		m_pcPInfo->m_nStatus = 0x80;

		uint nIndex = std::find( m_pcParent->m_cPartitionViews.begin(), m_pcParent->m_cPartitionViews.end(), this ) - m_pcParent->m_cPartitionViews.begin();
		nIndex += m_pcParent->m_nFirstVisible;
		
		for ( uint i = 0 ; i < m_pcParent->m_pcPList->size() ; ++i ) {
		    if ( i != nIndex ) {
			m_pcParent->m_cPartitionViews[i]->m_pcActiveCB->SetValue( false );
			m_pcParent->m_cPartitionViews[i]->m_pcPInfo->m_nStatus = 0x00;
		    }
		}
	    } else {
		m_pcPInfo->m_nStatus = 0x00;
	    }
	    break;
	case ID_DELETE_PARTITION:
	{
	    if ( m_pcPInfo == NULL ) {
		break;
	    }
	    int32 nButton = -1;
	    if ( pcMessage->FindInt32( "which", &nButton ) == 0 && nButton == 1 ) {
		SetPartitionType( 0, false );
	    }
	    break;
	}
	default:
	    LayoutView::HandleMessage( pcMessage );
	    break;
    }
}

/******************************************************************************
**** PartitionBar *************************************************************
******************************************************************************/


PartitionBar::PartitionBar( const Rect& cFrame, PartitionView* pcParent, const std::string& cName, float vStart, float vEnd ) :
	View( cFrame, cName, CF_FOLLOW_NONE, WID_FULL_UPDATE_ON_RESIZE )
{
    m_vStart = vStart;
    m_vEnd   = vEnd;
    m_vDragOffset  = 0.0f;
    m_bDragPointer = false;
    m_bDragStart   = false;
    m_bDragEnd     = false;
	m_nColor       = 0;
    m_pcParent     = pcParent;
}

void PartitionBar::AttachedToWindow()
{
}

Point PartitionBar::GetPreferredSize( bool bLargest ) const
{
    if ( bLargest ) {
	return( Point( 10000.0f, 16.0f ) );
    } else {
	return( Point( 30.0f, 16.0f ) );
    }
}

void PartitionBar::SetStart( float vValue )
{
    m_vStart = vValue;
    Invalidate();
    Flush();
}

void PartitionBar::SetEnd( float vValue )
{
    m_vEnd = vValue;
    Invalidate();
    Flush();
}

void PartitionBar::SetColor( uint32 nColor )
{
	m_nColor = nColor;
	Invalidate();
	Flush();
}

void PartitionBar::Paint( const Rect& cUpdateRect )
{
    Rect cBounds = GetNormalizedBounds();
    float vWidth = cBounds.Width() - 5.0f;
    
    DrawFrame( cBounds, FRAME_RECESSED | FRAME_TRANSPARENT );
    
    cBounds.Resize( 2.0f, 2.0f, -2.0f, -2.0f );

    if ( m_pcParent->GetType() == 0 ) {
	Color32_s sBgCol = get_default_color( COL_NORMAL );
	FillRect( cBounds, sBgCol );
    } else {
	float x1 = floor( 2.0f + vWidth * m_vStart );
	float x2 = floor( 2.0f + vWidth * m_vEnd );

	Color32_s sBgCol = get_default_color( COL_NORMAL );
	FillRect( Rect( 2.0f, cBounds.top, x1 - 1.0f, cBounds.bottom ), sBgCol );
	FillRect( Rect( x2 + 1.0f, cBounds.top, cBounds.right, cBounds.bottom ), sBgCol );
	if ( m_pcParent->IsValidExtended() ) {
	    if ( m_pcParent->IsMounted() ) {
		FillRect( Rect( x1, cBounds.top, x2, cBounds.bottom ), Color32_s( 100, 100, 100 ) );
	    } else {
		FillRect( Rect( x1, cBounds.top, x2, cBounds.bottom ), Color32_s( m_nColor ) );
	    }
	} else {
	    if ( m_pcParent->IsMounted() ) {
		FillRect( Rect( x1, cBounds.top, x2, cBounds.bottom ), Color32_s( 100, 100, 100 ) );
	    } else {
		FillRect( Rect( x1, cBounds.top, x2, cBounds.bottom ), Color32_s( m_nColor ) );
	    }
	}
    }
}

void PartitionBar::MouseMove( const Point& cNewPos, int nCode, uint32 nButtons, Message* pcData )
{
    if ( m_pcParent->GetType() == 0 || m_pcParent->IsMounted() ) {
	return;
    }    
    Rect cBounds = GetNormalizedBounds();

    if ( cBounds.Width() > 5.0f ) {
	if ( m_bDragStart ) {
	    m_vStart = (cNewPos.x - m_vDragOffset - 2.0f) / (cBounds.Width()-5.0f);
	    if ( m_vStart < 0.0f ) {
		m_vStart = 0.0f;
	    } else if ( m_vStart > m_vEnd ) {
		m_vStart = m_vEnd;
	    }
	    m_pcParent->SliderStartChanged( &m_vStart );
	    Invalidate();
	    Flush();
	} else 	if ( m_bDragEnd ) {
	    m_vEnd = (cNewPos.x - m_vDragOffset - 2.0f) / (cBounds.Width()-5.0f);
	    if ( m_vEnd < m_vStart ) {
		m_vEnd = m_vStart;
	    } else if ( m_vEnd > 1.0f ) {
		m_vEnd = 1.0f;
	    }
	    m_pcParent->SliderEndChanged( &m_vEnd );
	    Invalidate();
	    Flush();
	}
    }
    
    float vStart = 2.0f + (cBounds.Width()-5.0f) * m_vStart;
    float vEnd   = 2.0f + (cBounds.Width()-5.0f) * m_vEnd;

    
    if ( nCode != MOUSE_EXITED && cBounds.DoIntersect( cNewPos) && (fabs( cNewPos.x - vStart ) < 4.0f || fabs( cNewPos.x - vEnd ) < 4.0f) ) {
	if ( m_bDragPointer == false ) {
	    Application::GetInstance()->PushCursor( MPTR_MONO, g_anHMouseImg, H_POINTER_WIDTH, H_POINTER_HEIGHT,
						    IPoint( H_POINTER_WIDTH / 2, H_POINTER_HEIGHT / 2 ) );
	    m_bDragPointer = true;
	}
    } else if ( m_bDragPointer ) {
	Application::GetInstance()->PopCursor();
	m_bDragPointer = false;
    }
}

void PartitionBar::MouseDown( const Point& cPosition, uint32 nButton )
{
    if ( m_pcParent->GetType() == 0 || m_pcParent->IsMounted() ) {
	return;
    }    
    if ( nButton != 1 ) {
	View::MouseDown( cPosition, nButton );
	return;
    }
    Rect cBounds = GetNormalizedBounds();
    float vStart = 2.0f + (cBounds.Width()-5.0f) * m_vStart;
    float vEnd   = 2.0f + (cBounds.Width()-5.0f) * m_vEnd;

    if ( fabs( cPosition.x - vEnd ) < 4.0f ) {
	m_bDragEnd = true;
	m_vDragOffset = cPosition.x - vEnd;
    }
    if ( fabs( cPosition.x - vStart ) < 4.0f ) {
	m_bDragStart = true;
	m_vDragOffset = cPosition.x - vStart;
    }
    if ( m_bDragStart && m_bDragEnd ) {
	if ( cPosition.x < vStart + (vEnd-vStart)*0.5f ) {
	    m_bDragEnd = false;
	    m_vDragOffset = cPosition.x - vStart;
	} else {
	    m_bDragStart = false;
	    m_vDragOffset = cPosition.x - vEnd;
	}
    }
    MakeFocus();
}

void PartitionBar::MouseUp( const Point& cPosition, uint32 nButton, Message* pcData )
{
    if ( m_pcParent->GetType() == 0 || m_pcParent->IsMounted() ) {
	return;
    }    
    if ( nButton != 1 ) {
	View::MouseUp( cPosition, nButton, pcData );
	return;
    }
    m_bDragStart = m_bDragEnd = false;
}
