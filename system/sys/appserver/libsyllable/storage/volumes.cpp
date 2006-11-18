/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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
 
#include <atheos/types.h>
#include <atheos/kernel.h>
#include <atheos/image.h>
#include <atheos/cdrom.h>
#include <gui/requesters.h>
#include <gui/layoutview.h>
#include <gui/textview.h>
#include <gui/button.h>
#include <gui/treeview.h>
#include <gui/image.h>
#include <util/message.h>
#include <util/messenger.h>
#include <util/string.h>
#include <util/resources.h>
#include <util/thread.h>
#include <util/application.h>
#include <storage/path.h>
#include <storage/symlink.h>
#include <storage/directory.h>
#include <storage/volumes.h>
#include <storage/memfile.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cassert>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <string>

#include "catalogs/libsyllable.h"

#define GS( x, def )		( m_pcCatalog ? m_pcCatalog->GetString( x, def ) : def )
// For UnmountDialog
#define UGS( x, def )		( pcCatalog ? pcCatalog->GetString( x, def ) : def )

using namespace os;


static uint8 g_aCDImage[] = {
#include "../gui/pixmaps/cd.h"
};

static uint8 g_aDiskImage[] = {
#include "../gui/pixmaps/disk.h"
};

static uint8 g_aFloppyImage[] = {
#include "../gui/pixmaps/floppy.h"
};


struct MountDialogParams_s
{
	MountDialogParams_s( const Messenger & cViewTarget, Message* pcMsg )
	: m_cViewTarget( cViewTarget ), m_pcMsg( pcMsg )
	{
	}
	Messenger m_cViewTarget;
	Message* m_pcMsg;
};

class MountDialogWin;

class MountDialogScanner : public os::Thread
{
public:
	MountDialogScanner( MountDialogWin* pcWin );
	~MountDialogScanner();
	void ScanPath( os::Path cPath, int nDepth, MemFile* pcSource );
	bool CheckMounted( os::String zDevice );
	int32 Run();
private:
	MountDialogWin* m_pcWin;
	os::Catalog*	m_pcCatalog;
};

class MountDialogWin : public os::Window
{
public:
	MountDialogWin( MountDialogParams_s* psParams, os::Rect cFrame ) : os::Window( cFrame, "mount_window", 
																			"Mount" )
	{
		m_psParams = psParams;

		m_pcCatalog = os::Application::GetInstance()->GetApplicationLocale()->GetLocalizedSystemCatalog( "libsyllable.catalog" );

		SetTitle( GS( ID_MSG_MOUNT_WINDOW_TITLE, "Mount" ) );

		/* Create main view */
		m_pcView = new os::LayoutView( GetBounds(), "main_view" );
	
		os::VLayoutNode* pcVRoot = new os::VLayoutNode( "v_root" );
		pcVRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
		
	
		os::HLayoutNode* pcHNode = new os::HLayoutNode( "h_node" );
		
		/* Create disks view */
		m_pcDisks = new os::TreeView( os::Rect(), "disks", 
							os::ListView::F_RENDER_BORDER, os::CF_FOLLOW_ALL, os::WID_FULL_UPDATE_ON_RESIZE );
		m_pcDisks->InsertColumn( GS( ID_MSG_MOUNT_WINDOW_NAME_COLUMN, "Name" ).c_str(), (int)GetBounds().Width() - 150 );
		m_pcDisks->InsertColumn( GS( ID_MSG_MOUNT_WINDOW_PATH_COLUMN, "Path" ).c_str(), 150 );
		m_pcDisks->SetAutoSort( false );
		//m_pcDisks->SetInvokeMsg( new os::Message( M_LIST_INVOKED ) );
		m_pcDisks->SetTarget( this );
		m_pcDisks->SetInvokeMsg( new os::Message( 1 ) );
	//	m_pcDisks->SetDrawTrunk( true );
//		m_pcDisks->SetDrawExpanderBox( false );
		//m_pcList->SetExpandedImage( LoadImage( "expander.png" ) );
	    //m_pcList->SetCollapsedImage( LoadImage( "collapse.png" ) );
		pcVRoot->AddChild( m_pcDisks );
		pcVRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	
		/* create input view */
		m_pcInput = new os::TextView( os::Rect(), "input", "Name", 
								os::CF_FOLLOW_LEFT | os::CF_FOLLOW_RIGHT);
		m_pcInput->SetReadOnly( false );
		m_pcInput->SetMultiLine( false );
		m_pcInput->Set( "" );
		pcHNode->AddChild( m_pcInput ); 
		pcHNode->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
		
	
		/* create buttons */
		m_pcMount = new os::Button( os::Rect(), "mount", 
						GS( ID_MSG_MOUNT_WINDOW_MOUNT_BUTTON, "Mount" ), new os::Message( 0 ), os::CF_FOLLOW_RIGHT | os::CF_FOLLOW_BOTTOM );
	
		
		pcHNode->AddChild( m_pcMount )->LimitMaxSize( m_pcMount->GetPreferredSize( false ) );
		pcHNode->SameHeight( "input", "mount", NULL );
		
		pcVRoot->AddChild( pcHNode );
		
		m_pcView->SetRoot( pcVRoot );
	
		AddChild( m_pcView );
		
		m_pcInput->SetTabOrder( 0 );
		m_pcMount->SetTabOrder( 1 );
		m_pcDisks->SetTabOrder( 2 );
		
		SetDefaultButton( m_pcMount );
		
		m_pcInput->MakeFocus();
		
	}
	

	~MountDialogWin() { 
		delete( m_psParams->m_pcMsg );
		delete( m_psParams );
		if( m_pcCatalog )
			m_pcCatalog->Release();
	}
	
	void StartScanner()
	{
		m_pcScanner = new MountDialogScanner( this );
		m_pcScanner->Start();
	}
	
	void Mount( os::String zPath )
	{
		os::String zDevice = zPath;
		os::String zMountPoint = "/Unknown";
		fs_info sInfo;
		
		/* Check if mounted between the scan and now */
		if( m_pcScanner->CheckMounted( zPath ) )
			return;
			
		
		/* Try to read the volume name */
		if( probe_fs( zDevice.c_str(), &sInfo ) == 0 && strlen( sInfo.fi_volume_name ) > 0 &&
			strcmp( sInfo.fi_volume_name, "/" ) )
		{
			zMountPoint = os::String( "/" ) + os::String( sInfo.fi_volume_name );
		}
		
		
		/* Create dir */
		try
		{
			
			bool bCreated = false;
			os::Directory cDir( "/" );
			os::String zNewDir;
			for( uint i = 0; i < 10; i++ )
			{
				char zPosition[10];
				sprintf( zPosition, ".%i", i );
				if( i == 0 )
					zNewDir = zMountPoint;
				else
					zNewDir = zMountPoint + os::String( zPosition );
				//printf( "Trying moutpoint %s\n", zNewDir.c_str() );
				if( cDir.CreateDirectory( zNewDir, &cDir, S_IRWXU|S_IRWXG|S_IRWXO ) >= 0 )
				{
					bCreated = true;
					break;
				}
			}
			if( !bCreated )
			{
				os::String cErrMsg;
				cErrMsg.Format( GS( ID_MSG_MOUNT_ERROR_CREATE_MOUNTPOINT, "Could not mount create mountpoint for device %s" ).c_str(), zDevice.c_str() );
				os::Alert* pcAlert = new os::Alert( GS( ID_MSG_MOUNT_ERROR_TITLE, "Mount" ), cErrMsg,
													os::Alert::ALERT_WARNING, 0, GS( ID_MSG_MOUNT_ERROR_CLOSE, "Ok" ).c_str(), NULL );
				pcAlert->Go( new os::Invoker( 0 ) );
				return;	
			}
			
			/* Mount */
			if( mount( zDevice.c_str(), zNewDir.c_str(), NULL, 0, NULL ) < 0 ) {
				rmdir( zPath.c_str() );
				os::String cErrMsg;
				cErrMsg.Format( GS( ID_MSG_MOUNT_ERROR_OTHER_ERROR, "Could not mount %s:\n%s" ).c_str(), zDevice.c_str(), strerror( errno ) );
				os::Alert* pcAlert = new os::Alert( GS( ID_MSG_MOUNT_ERROR_TITLE, "Mount" ), cErrMsg,
													os::Alert::ALERT_WARNING, 0, GS( ID_MSG_MOUNT_ERROR_CLOSE, "Ok" ).c_str(), NULL );
				pcAlert->Go( new os::Invoker( 0 ) );
			}
			else {
				m_psParams->m_cViewTarget.SendMessage( m_psParams->m_pcMsg );
				PostMessage( os::M_QUIT, this );
			}
			
			
		} catch( ... ) {
			return;
		}
		
	}

	void HandleMessage( os::Message* pcMessage )
	{
		switch( pcMessage->GetCode() )
		{
			
			case 0:
			{
				/* Look if the input field contains anything */
				if( !( m_pcInput->GetBuffer()[0] == "" ) )
				{
					Mount( m_pcInput->GetBuffer()[0] );
					//printf( "%s\n", m_pcInput->GetBuffer()[0].c_str() );
					break;
					
				}
				/* Fall through */	
			}
			case 1:
			{
				
				/* Take selected object */
				int nSelected = m_pcDisks->GetFirstSelected();
				
				if( nSelected > -1 )
				{
					os::TreeViewStringNode* pcNode = static_cast<os::TreeViewStringNode*>(m_pcDisks->GetRow( nSelected ));
					
					if( !( pcNode->GetString( 1 ) == "" ) )
						Mount( pcNode->GetString( 1 ) );
					
					//printf( "%s\n", pcNode->GetString( 1 ).c_str() );
					
				}
				break;
			}
			
			default:
				break;
		}
		os::Window::HandleMessage( pcMessage );
	}
	
private:
	friend class MountDialogScanner;
	MountDialogParams_s* m_psParams;
	MountDialogScanner* m_pcScanner;
	os::LayoutView*		m_pcView;
	os::Button*			m_pcMount;
	os::TreeView*		m_pcDisks;
	os::TextView*		m_pcInput;
public:
	os::Catalog*		m_pcCatalog;
};

MountDialogScanner::MountDialogScanner( MountDialogWin* pcWin )
					: os::Thread( "mount_scanner" )
{
	m_pcWin = pcWin;
	m_pcCatalog = pcWin->m_pcCatalog;
	if( m_pcCatalog )
		m_pcCatalog->AddRef();
}

MountDialogScanner::~MountDialogScanner()
{
	if( m_pcCatalog )
		m_pcCatalog->Release();
}
	
bool MountDialogScanner::CheckMounted( os::String zDevice )
{
	
	/* Check if one device is mounted */
	char zTemp[PATH_MAX];
	for( int i = 0; i < get_mount_point_count(); i++ )
	{
		if( get_mount_point( i, zTemp, PATH_MAX ) == 0 ) {
			int nFD = open( zTemp, O_RDONLY );
			fs_info sInfo;
			
			
			if( nFD )
			{
				if( get_fs_info( nFD, &sInfo ) == 0 && ( sInfo.fi_flags & FS_IS_BLOCKBASED ) )
				{
					//printf( "%s %s %s\n", sInfo.fi_device_path, zTemp, zDevice.c_str() );
			
					if( os::String( sInfo.fi_device_path ) == zDevice ) {
						close( nFD );
						return( true );
					}
				}
				close( nFD );
			}
		}
	}
	return( false );
}
		
void MountDialogScanner::ScanPath( os::Path cPath, int nDepth, MemFile* pcSource )
{
	/* Scan one directory for disks */
	try
	{
		os::Directory cDir( cPath.GetPath() );
		os::String zEntry;
		os::Path cEntryPath;

		os::String cScanMsg;
		cScanMsg.Format( GS( ID_MSG_MOUNT_SCANNING_PATH, "Scanning %s..." ).c_str(), cPath.GetPath().c_str() );		
		m_pcWin->m_pcInput->Set( cScanMsg.c_str() );

		while( cDir.GetNextEntry( &zEntry ) == 1 )
		{
			if( zEntry == "." || zEntry == ".." )
				continue;
				
			cEntryPath = cPath;
			cEntryPath.Append( zEntry );
			
			
			try
			{
				struct stat sStat;
				if( lstat( cEntryPath.GetPath().c_str(), &sStat ) >= 0 && S_ISDIR( sStat.st_mode ) )
				{
					MemFile* pcSource = NULL;
					
					/* Add directory entry */
					if( nDepth == 1 )
					{
						os::String zName;
						
						/* Check raw */
						os::String cRawPath = cEntryPath.GetPath() + os::String( "/raw" );
						if( lstat( cRawPath.c_str(), &sStat ) < 0 )
							break;
						
						/* Create name and icon */
						if( zEntry[0] == 'h' && zEntry[1] == 'd' ) {
							zName = GS( ID_MSG_MOUNT_TYPE_HARDDISK, "Harddisk" ) + os::String( " (" ) + zEntry + os::String( ")" );
							pcSource = new MemFile( g_aDiskImage, sizeof( g_aDiskImage ) );
						}
						else if( zEntry[0] == 'c' && zEntry[1] == 'd' ) {
							zName = GS( ID_MSG_MOUNT_TYPE_CD_DVD, "CD/DVD" ) + os::String( " (" ) + zEntry + os::String( ")" );
							pcSource = new MemFile( g_aCDImage, sizeof( g_aCDImage ) );
						}
						else if( zEntry[0] == 'f' && zEntry[1] == 'd' ) {
							zName = GS( ID_MSG_MOUNT_TYPE_FLOPPY, "Floppy" ) + os::String( " (" ) + zEntry + os::String( ")" );
							pcSource = new MemFile( g_aFloppyImage, sizeof( g_aFloppyImage ) );
						}
						else {
							zName = zEntry;
							pcSource = new MemFile( g_aDiskImage, sizeof( g_aDiskImage ) );
						}
						
						/* Load icon */
						os::BitmapImage* pcIcon = new os::BitmapImage();
						
						pcIcon->Load( pcSource );
						pcIcon->SetSize( os::Point( 24, 24 ) );
												
						/* Create directory node */
						os::TreeViewStringNode* pcNewRow = new os::TreeViewStringNode();
						pcNewRow->AppendString( zName );
						
						if( CheckMounted( cEntryPath.GetPath() + os::String( "/raw" ) ) )
							pcNewRow->AppendString( "" );
						else
							pcNewRow->AppendString( cEntryPath.GetPath() + os::String( "/raw" ) );
						pcNewRow->SetIndent( 2 );
						pcNewRow->SetIcon( pcIcon );
						m_pcWin->Lock();
						m_pcWin->m_pcDisks->InsertNode( pcNewRow );
						m_pcWin->Unlock();
					}
			
					/* Recursive scan */
					ScanPath( cEntryPath, nDepth + 1, pcSource );
					
					//if( nDepth == 1 )
						//delete( pcSource );
					
					continue;
				} 
			} catch( ... ) {}
			
			if( zEntry == "raw" )
				continue;
				
			//printf( "%s\n", cEntryPath.GetPath().c_str() );
				
				
			if( CheckMounted( cEntryPath.GetPath() ) )
				continue;
				
			//printf( "%s\n", cEntryPath.GetPath().c_str() );
				
			/* Load icon */
			os::BitmapImage* pcIcon = new os::BitmapImage();
			pcSource->Seek( 0, SEEK_SET );
			pcIcon->Load( pcSource );
			pcIcon->SetSize( os::Point( 24, 24 ) );

			/* Add partition entry */
			os::TreeViewStringNode* pcNewRow = new os::TreeViewStringNode();
			pcNewRow->AppendString( GS( ID_MSG_MOUNT_DEVICE_PARTITION, "Partition" ) + os::String( " " ) + zEntry );
			pcNewRow->AppendString( cEntryPath.GetPath() );
			pcNewRow->SetIndent( 3 );
			pcNewRow->SetIcon( pcIcon );
			m_pcWin->Lock();
			m_pcWin->m_pcDisks->InsertNode( pcNewRow );
			m_pcWin->Unlock();
		}
	} catch( ... ) {
		return;
	}
}
	
int32 MountDialogScanner::Run()
{
	m_pcWin->Lock();
	m_pcWin->m_pcDisks->Clear();
	m_pcWin->m_pcDisks->Hide();
	m_pcWin->m_pcInput->Set( GS( ID_MSG_MOUNT_SCANNING, "Scanning..." ).c_str() );
	m_pcWin->m_pcInput->SetEnable( false );
	m_pcWin->m_pcMount->SetEnable( false );
	
	
	os::TreeViewStringNode *pcNewRow;
		
	/* Add local disks */
	pcNewRow = new os::TreeViewStringNode();
	pcNewRow->AppendString( GS( ID_MSG_MOUNT_LOCAL_DISKS_LABEL, "Local disks" ) );
	pcNewRow->AppendString( "" );
	pcNewRow->SetIndent( 1 );
	m_pcWin->m_pcDisks->InsertNode( pcNewRow );
	m_pcWin->Unlock();
	ScanPath( os::Path( "/dev/disk" ), 0, NULL );
		
	m_pcWin->Lock();
	m_pcWin->m_pcDisks->Show();
	m_pcWin->m_pcInput->Set( "" );
	m_pcWin->m_pcInput->SetEnable( true );
	m_pcWin->m_pcMount->SetEnable( true );
	m_pcWin->Unlock();
	
	return( 0 );
}


struct UnmountDialogParams_s
{
	UnmountDialogParams_s( os::String zPath, const Messenger & cViewTarget, Message* pcMsg )
	:m_cViewTarget( cViewTarget ), m_pcMsg( pcMsg ), m_zPath( zPath )
	{
	}
	Messenger m_cViewTarget;
	Message* m_pcMsg;
	os::String m_zPath;
};

int32 UnmountThread( void *pData )
{
	bool bEject = false;
	UnmountDialogParams_s *psArgs = ( UnmountDialogParams_s * ) pData;
	
	/* Search device and check if it is a cdrom drive */
	os::String zDevice = psArgs->m_zPath;
	
	int nFD = open( psArgs->m_zPath.c_str(), O_RDONLY );
	fs_info sInfo;
			
	if( nFD )
	{
		if( get_fs_info( nFD, &sInfo ) == 0 && ( sInfo.fi_flags & FS_IS_BLOCKBASED ) )
		{
			zDevice = sInfo.fi_device_path; /* Copy device path */
			os::String zTemp = os::Path( zDevice ).GetDir().GetLeaf();
			if( zTemp.Length() > 2 )
				if( zTemp[0] == 'c' && zTemp[1] == 'd' )
					bEject = true;
		}
		close( nFD );
	}
	
	
	if( unmount( psArgs->m_zPath.c_str(), false ) < 0 )
	{
		os::String cErrMsg;
		os::Catalog* pcCatalog = os::Application::GetInstance()->GetApplicationLocale()->GetLocalizedSystemCatalog( "libsyllable.catalog" );

		cErrMsg.Format( UGS( ID_MSG_MOUNT_ERROR_UNMOUNT, "Could not unmount %s:\n%s" ).c_str(), zDevice.c_str(), strerror( errno ) );
		os::Alert* pcAlert = new os::Alert( UGS( ID_MSG_MOUNT_ERROR_TITLE, "Mount" ), cErrMsg, 
												os::Alert::ALERT_WARNING, 0, UGS( ID_MSG_MOUNT_ERROR_CLOSE, "Ok" ).c_str(), NULL );	

		if( pcCatalog )
			pcCatalog->Release();

		pcAlert->Go( new os::Invoker( 0 ) );
		goto out;
	}
	
	rmdir( psArgs->m_zPath.c_str() );
	
	if( bEject )
	{
		//printf( "Eject %s\n", zDevice.c_str() );
		/* Eject */
		nFD = open( zDevice.c_str(), O_RDWR );
		if( nFD )
		{
			ioctl( nFD, CD_EJECT, NULL );
			close( nFD );
		}
	}
				
out:
	psArgs->m_cViewTarget.SendMessage( psArgs->m_pcMsg );

	delete( psArgs->m_pcMsg );
	delete( psArgs );

	return ( 0 );
}



Volumes::Volumes()
{
}

Volumes::~Volumes()
{
}


/** Return the number of mount points.
 * \par Description:
 *	Returns the number of mount points.
 *
 * \return The number of mount points.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
uint Volumes::GetMountPointCount()
{
	return( get_mount_point_count() );
}


/** Return a mountpoint.
 * \par Description:
 *	After a successful call of this method, cMountPoint will contain the path
 *  of the mountpoint specified by nIndex.
 * \param nIndex - Index of the mountpoint.
 * \param cMountPoint - Will contain the path of the mountpoint after the call.
 * \return 0 if successful.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t Volumes::GetMountPoint( uint nIndex, String* cMountPoint )
{
	char zTemp[PATH_MAX];
	if( get_mount_point( nIndex, zTemp, PATH_MAX ) < 0 )
		return( -1 );
	*cMountPoint = zTemp;
	return( 0 );
}


/** Return information about a mounted filesystem.
 * \par Description:
 *	After a successful call of this method, psInfo will contain filesystem
 *  related information about the mountpoint specified by nIndex.
 * \param nIndex - Index of the mountpoint.
 * \param psInfo - Pointer to a fs_info structure.
 * \return 0 if successful.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
status_t Volumes::GetFSInfo( uint nIndex, fs_info* psInfo )
{
	char zTemp[PATH_MAX];
	if( get_mount_point( nIndex, zTemp, PATH_MAX ) < 0 )
		return( -1 );
	int nFD = open( zTemp, O_RDONLY );
				
	if( nFD )
	{
		status_t nError = get_fs_info( nFD, psInfo );
		close( nFD );
		return( nError );
	}
	return( -1 );
}


/** Create a mount dialog.
 * \par Description:
 *	Creates a dialog which lists all volumes that can be mounted and lets the
 *  user select one. When a volume is mounted, pcMountMsg will be sent to the
 *  supplied message target.
 * \par Note:
 *	Don’t forget to place the window at the right position and show it.
 * \param cMsgTarget - The target that will receive the message.
 * \param cMountMsg - The message that will be sent.
 * \return Pointer to the created window.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
Window* Volumes::MountDialog( const Messenger & cMsgTarget, Message* pcMountMsg )
{
	MountDialogParams_s *psParams = new MountDialogParams_s( cMsgTarget, pcMountMsg );
	MountDialogWin* pcWin = new MountDialogWin( psParams, os::Rect( 0, 0, 300, 250 ) );
	
	pcWin->StartScanner();
	
	return( pcWin );
}


/** Unmounts a volume.
 * \par Description:
 *	Unmounts the volume specified by cMountPoint. After the volume has
 *  been unmounted, pcFinish will be sent to the supplied message target.
 * \param cMountPoint - The mountpoint.
 * \param cMsgTarget - The target that will receive the message.
 * \param cMountMsg - The message that will be sent.
 *
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void Volumes::Unmount( String cMountPoint, const Messenger & cMsgTarget, Message* pcFinishMsg )
{
	UnmountDialogParams_s *psParams = new UnmountDialogParams_s( cMountPoint, cMsgTarget, pcFinishMsg );
	thread_id hThread = spawn_thread( "unmount_thread", (void*)UnmountThread, 0, 0, psParams );

	if( hThread >= 0 )
	{
		resume_thread( hThread );
	}
}
