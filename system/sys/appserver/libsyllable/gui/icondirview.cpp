/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <cassert>
#include <sys/stat.h>

#include <atheos/time.h>

#include <gui/window.h>
#include <gui/bitmap.h>
#include <gui/image.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/iconview.h>
#include <gui/icondirview.h>
#include <gui/menu.h>
#include <gui/font.h>
#include <util/application.h>
#include <util/looper.h>
#include <util/message.h>
#include <util/event.h>
#include <storage/registrar.h>
#include <storage/directory.h>
#include <storage/nodemonitor.h>
#include <storage/symlink.h>
#include <storage/filereference.h>
#include <storage/operations.h>
#include <storage/volumes.h>
#include <storage/memfile.h>
#include <storage/file.h>

#include "catalogs/libsyllable.h"

#include <list>
#include <set>
//#include <iostream>

#define LAYOUT_TIME 800000

static uint8 g_aCDImage[] = {
#include "pixmaps/cd.h"
};

static uint8 g_aDiskImage[] = {
#include "pixmaps/disk.h"
};

static uint8 g_aFloppyImage[] = {
#include "pixmaps/floppy.h"
};

uint8 g_aFileImage[] = {
#include "pixmaps/file.h"
};

uint8 g_aFolderImage[] = {
#include "pixmaps/folder.h"
};

using namespace os;

namespace os_priv
{
	

	class DirKeeper:public Looper
	{
	    public:
		enum
		{ M_CHANGE_DIR = 1, M_COPY_FILES, M_MOVE_FILES, M_DELETE_FILES, M_RENAME_FILES, M_ENTRIES_ADDED, M_ENTRIES_REMOVED, M_ENTRIES_UPDATED, M_LAYOUT_UPDATED };
		  DirKeeper( const Messenger & cTarget, const String& cPath );
		virtual void HandleMessage( Message * pcMessage );
		virtual bool Idle();
		void Stop();
		status_t GetNode( os::String zPath, os::FSNode* pcNode, bool *pbBrokenLink );
		Directory* GetCurrentDir() { return( m_pcCurrentDir ); }
		bool IsReading() { return( m_eState == S_READDIR ); }
   private:
		
		void SendAddMsg( const String& cName, dev_t nDevice, ino_t nInode );
		void SendRemoveMsg( const String& cName, dev_t nDevice, ino_t nInode );
		void SendUpdateMsg( const String& cName, dev_t nDevice, ino_t nInode, bool bReloadIcon );
		void SendDriveAddMsg( const String& cName, const String& cPath );

		enum state_t
		{ S_IDLE, S_READDIR };
		bool m_bRunning;
		Messenger m_cTarget;
		os::String m_cPath;
		Directory *m_pcCurrentDir;
		NodeMonitor m_cMonitor;
		state_t m_eState;
		int m_nPendingReplies;
		bool m_bLayoutNecessary;
		bool m_bWaitForLayoutReply;
		bigtime_t m_nLastLayout;
	};
}

using namespace os_priv;


DirKeeper::DirKeeper( const Messenger & cTarget, const String& cPath ):Looper( "dir_worker" ), m_cTarget( cTarget )
{
	m_bRunning = false;
	m_pcCurrentDir = NULL;
	try
	{
		m_cPath = cPath;
		m_pcCurrentDir = new Directory( cPath );
		m_pcCurrentDir->Rewind();
		m_cMonitor.SetTo( m_pcCurrentDir, NWATCH_DIR | NWATCH_NAME, this, this );
	} catch( std::exception & )
	{
	}
	m_eState = S_IDLE;
	m_nPendingReplies = 0;
	m_bLayoutNecessary = false;
	m_bWaitForLayoutReply = false;
	m_nLastLayout = get_system_time() - LAYOUT_TIME;
}

status_t DirKeeper::GetNode( os::String zPath, os::FSNode* pcNode, bool* pbBrokenLink )
{	
	if( pcNode->SetTo( *m_pcCurrentDir, zPath, O_RDONLY | O_NONBLOCK ) >= 0 )
	{
		*pbBrokenLink = false;
		return( 0 );
	}
	
	if( pcNode->SetTo( *m_pcCurrentDir, zPath, O_RDONLY | O_NONBLOCK | O_NOTRAVERSE ) >= 0 )
	{
		*pbBrokenLink = true;		
		return( 0 );
	}
		
	return( -EIO );
}

void DirKeeper::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case M_CHANGE_DIR:
		{
			String cNewPath;
			pcMessage->FindString( "path", &cNewPath );
			try
			{
				Stop();
				m_cPath = cNewPath;
				Directory *pcNewDir = new Directory( cNewPath );
				delete m_pcCurrentDir;

				m_pcCurrentDir = pcNewDir;
				m_pcCurrentDir->Rewind();
				
				m_eState = S_READDIR;
				if( !( cNewPath == "/" ) )
					m_cMonitor.SetTo( m_pcCurrentDir, NWATCH_DIR | NWATCH_NAME, this, this );
				else
					m_cMonitor.Unset();
				
			}
			catch( std::exception & )
			{
			}
			
			break;
		}
	case M_ENTRIES_ADDED:
	case M_ENTRIES_UPDATED:
	case M_ENTRIES_REMOVED:		
		{
			m_nPendingReplies--;
			//printf("Pending now %i\n", m_nPendingReplies );
			break;
		}
	case M_LAYOUT_UPDATED:
		{
			m_bWaitForLayoutReply = false;
			m_nLastLayout = get_system_time();
			break;
		}
	
	case M_NODE_MONITOR:
		{
			int32 nEvent = -1;

			pcMessage->FindInt32( "event", &nEvent );
			switch ( nEvent )
			{
			case NWEVENT_CREATED:
				{
					String cName;
					dev_t nDevice;
					int64 nInode;
					
					//std::cout<<"CREATE"<<std::endl;

					pcMessage->FindInt( "device", &nDevice );
					pcMessage->FindInt64( "node", &nInode );
					pcMessage->FindString( "name", &cName );
					SendAddMsg( cName, nDevice, nInode );
					break;
				}
			case NWEVENT_DELETED:
				{
					dev_t nDevice;
					int64 nInode;
					//std::cout<<"DELETE"<<std::endl;

					String cName;

					pcMessage->FindInt( "device", &nDevice );
					pcMessage->FindInt64( "node", &nInode );
					pcMessage->FindString( "name", &cName );
					SendRemoveMsg( cName, nDevice, nInode );
					break;
				}
			case NWEVENT_STAT_CHANGED:
				{
					dev_t nDevice;
					int64 nInode;

					String cName;

					pcMessage->FindInt( "device", &nDevice );
					pcMessage->FindInt64( "node", &nInode );
					pcMessage->FindString( "name", &cName );
					SendUpdateMsg( cName, nDevice, nInode, false );
					break;
				}
			case NWEVENT_ATTR_WRITTEN:
			case NWEVENT_ATTR_DELETED:
				{
				/* Not implemented yet in the kernel/glibc */
				break;
				}
			case NWEVENT_MOVED:
				{
					dev_t nDevice;
					int64 nOldDir;
					int64 nNewDir;
					int64 nInode;

					String cName;

					pcMessage->FindInt( "device", &nDevice );
					pcMessage->FindInt64( "old_dir", &nOldDir );
					pcMessage->FindInt64( "new_dir", &nNewDir );
					pcMessage->FindInt64( "node", &nInode );
					pcMessage->FindString( "name", &cName );


					if( nInode == int64 ( m_pcCurrentDir->GetInode() ) )
					{
						printf( "%Ld moved from %Ld to %Ld\n", nInode, nOldDir, nNewDir );
					}
					else
					{
						if( nOldDir == nNewDir )
						{
							SendUpdateMsg( cName, nDevice, nInode, true );
						}
						else
						{
							if( nOldDir == int64 ( m_pcCurrentDir->GetInode() ) )
							{
								SendRemoveMsg( cName, nDevice, nInode );
							}
							else
							{
								SendAddMsg( cName, nDevice, nInode );
							}
						}
					}
					break;
				}
			}
		}
	default:
		Looper::HandleMessage( pcMessage );
		break;
	}
}

void DirKeeper::SendUpdateMsg( const String& cName, dev_t nDevice, ino_t nInode, bool bReloadIcon )
{
	//std::cout<<"Update "<<cName.c_str()<<std::endl;
	try
	{
		
		bool bBrokenLink;
		struct stat sStat;
		
		if( cName == "" )
		{
			bBrokenLink = false;
			sStat.st_dev = nDevice;
			sStat.st_ino = nInode;
		} else
		{
			FSNode cNode;
			if( GetNode( cName, &cNode, &bBrokenLink ) < 0 )
				return;
			cNode.GetStat( &sStat, false );
		}
			
		
		//std::cout<<"Update "<<cName<<" "<<sStat.st_dev<<" "<<sStat.st_ino<<" "<<nInode<<std::endl;


		//FileNode cFileNode( cName, cName, sStat, bBrokenLink, bReloadIcon );
		
		Message cMsg( IconDirectoryView::M_UPDATE_ENTRY );
		cMsg.AddString( "name", cName );
		cMsg.AddString( "path", cName );
		cMsg.AddData( "stat", T_ANY_TYPE, &sStat, sizeof( sStat ) );
		cMsg.AddBool( "broken_link", bBrokenLink );
		cMsg.AddBool( "reload_icon", bReloadIcon );
			
			

		m_nPendingReplies++;
		m_cTarget.SendMessage( &cMsg );
		//printf("Send Update %s %i %i!\n", cName.c_str(), (int)nInode, m_nPendingReplies);
	} catch( std::exception & )
	{
		m_cTarget.SendMessage( IconDirectoryView::M_REREAD );
	}
	m_bLayoutNecessary = true;
}

void DirKeeper::SendAddMsg( const String& cName, dev_t nDevice, ino_t nInode )
{
	try
	{
		
		FSNode cNode;
		bool bBrokenLink = true;
		os::NodeMonitor* pcMonitor = NULL;
		struct stat sStat;
		memset( &sStat, 0, sizeof( sStat ) );
		
		if( GetNode( cName, &cNode, &bBrokenLink ) >= 0 )
			cNode.GetStat( &sStat, false );
		else {
			if( nDevice == -1 && nInode == -1 )
				return;
			sStat.st_dev = nDevice;
			sStat.st_ino = nInode;
		}
		if( !bBrokenLink )
			pcMonitor = new os::NodeMonitor( &cNode, NWATCH_STAT | NWATCH_ATTR, this, this );

		//std::cout<<"Add "<<sStat.st_dev<<" "<<sStat.st_ino<<std::endl;
		
		
		Message cMsg( IconDirectoryView::M_ADD_ENTRY );
		cMsg.AddString( "name", cName );
		cMsg.AddString( "path", cName );
		cMsg.AddData( "stat", T_ANY_TYPE, &sStat, sizeof( sStat ) );
		cMsg.AddBool( "broken_link", bBrokenLink );
		cMsg.AddPointer( "node_monitor", pcMonitor );
		
		
		m_nPendingReplies++;
		m_cTarget.SendMessage( &cMsg );
		//printf("Send add %s %i %i!\n", cName.c_str(), (int)sStat.st_ino, m_nPendingReplies);
	}
	catch( ... /*std::exception& */  )
	{
	}
	m_bLayoutNecessary = true;
}

void DirKeeper::SendRemoveMsg( const String& cName, dev_t nDevice, ino_t nInode )
{
	struct stat sStat;
	sStat.st_dev = nDevice;
	sStat.st_ino = nInode;
	
	
	Message cMsg( IconDirectoryView::M_REMOVE_ENTRY );
	cMsg.AddInt32( "device", nDevice );
	cMsg.AddInt64( "node", nInode );
	
	
	m_nPendingReplies++;
	m_cTarget.SendMessage( &cMsg );
	m_bLayoutNecessary = true;
	//printf("Send Remove %i %i!\n", (int)nInode, m_nPendingReplies);
}


void DirKeeper::SendDriveAddMsg( const String& cName, const String& cPath )
{
	try
	{
		
		FSNode cNode( *m_pcCurrentDir, cPath, O_RDONLY | O_NONBLOCK );
		struct stat sStat;

		cNode.GetStat( &sStat, false );

		
		Message cMsg( IconDirectoryView::M_ADD_ENTRY );
		cMsg.AddString( "name", cName );
		cMsg.AddString( "path", cPath );
		cMsg.AddData( "stat", T_ANY_TYPE, &sStat, sizeof( sStat ) );
		cMsg.AddBool( "broken_link", false );
		cMsg.AddPointer( "node_monitor", NULL );

		m_nPendingReplies++;
		m_cTarget.SendMessage( &cMsg );
		
	}
	catch( ... /*std::exception& */  )
	{
	}
	m_bLayoutNecessary = true;
}

void DirKeeper::Stop()
{
	Lock();
	m_eState = S_IDLE;
	m_bLayoutNecessary = false;
	m_bWaitForLayoutReply = false;
	m_cMonitor.Unset();
	
	m_nPendingReplies = 0;
	
	/* Make sure we delete all pending messages */
	SpoolMessages();	// Copy all messages from the message port to the message queue.
	MessageQueue *pcQueue = GetMessageQueue();
	MessageQueue cTmp;
	Message* pcMsg;
	pcQueue->Lock();
	while( ( pcMsg = pcQueue->NextMessage() ) != NULL )
	{
		switch( pcMsg->GetCode() )
		{
			case M_ENTRIES_ADDED:
			case M_ENTRIES_UPDATED:
			case M_ENTRIES_REMOVED:
			case M_NODE_MONITOR:
				break;
			default:
				cTmp.AddMessage( pcMsg );
		}
	}
	
	while( ( pcMsg = cTmp.NextMessage() ) != NULL )
	{
		pcQueue->AddMessage( pcMsg );
	}
	
	pcQueue->Unlock();
	Unlock();
}

bool DirKeeper::Idle()
{
	switch ( m_eState )
	{
	case S_READDIR:
		{
			if( m_pcCurrentDir == NULL )
			{
				m_eState = S_IDLE;
				return ( true );
			}
			os::String cName;
			
			if( m_cPath == "/" )
			{
				/* Show drives */
				os::Volumes cVolumes;
				os::String cMountPoint;
				//char zTemp[PATH_MAX];
				for( uint i = 0; i < cVolumes.GetMountPointCount(); i++ )
				{
					if( cVolumes.GetMountPoint( i, &cMountPoint ) == 0 ) {
						fs_info sInfo;
						if( cVolumes.GetFSInfo( i, &sInfo ) == 0 && ( sInfo.fi_flags & FS_IS_BLOCKBASED ) )
						{
						
							os::String zVolumeName = sInfo.fi_volume_name;
							if( ( zVolumeName == "" ) || ( zVolumeName == "/" ) )
								zVolumeName = "Unknown";
								
							/* Remove '/' */
							os::String cVolumePath = cMountPoint.substr( 1, cMountPoint.Length() - 1 );
							SendDriveAddMsg( zVolumeName, cVolumePath );
						}
					}
				}
				m_eState = S_IDLE;
				return ( true );
			} 
			else
			{
				/* Show files */
				if( m_bLayoutNecessary )
				{
					if( m_nPendingReplies > 0 )
					{
						if( get_system_time() - m_nLastLayout > LAYOUT_TIME && !m_bWaitForLayoutReply )
						{
							/* Update regulary */
							Message cMsg( IconDirectoryView::M_LAYOUT );
							m_cTarget.SendMessage( &cMsg );
							m_bWaitForLayoutReply = true;
							m_bLayoutNecessary = false;
							m_nLastLayout = get_system_time();
						}
					}
				}
			
				if( m_nPendingReplies > 5 )
					return( false );
					
				if( m_pcCurrentDir->GetNextEntry( &cName ) == 1 )
				{
					if( !( cName == "." ) && !( cName == ".." ) )
					{
						SendAddMsg( cName, -1, -1 );
					}
					return ( true );
				}
				else
				{
					m_eState = S_IDLE;
					return ( true );
				}
			}
			break;
		}
	case S_IDLE:
		if( m_bLayoutNecessary )
		{
			if( m_nPendingReplies > 0 )
				return( false );

			/* Final layout */			
			Message cMsg( IconDirectoryView::M_LAYOUT );
			m_cTarget.SendMessage( &cMsg );
			m_nLastLayout = get_system_time();
			m_bWaitForLayoutReply = true;
			m_bLayoutNecessary = false;
		}
		return ( false );
	}
	
	
	return ( false );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DirectoryIconData::~DirectoryIconData()
{
	delete m_pcMonitor;
}

class IconDirectoryView::Private
{
public:
	os::BitmapImage* GetDriveIcon( os::String zPath, fs_info* psInfo, bool bSmall ) const;
	
	Message*	        m_pcDirChangeMsg;
	Path	        	m_cPath;
	os_priv::DirKeeper* m_pcDirKeeper;
	bool				m_bLocked;
	bool				m_bAutoLaunch;
	os::Menu*			m_pcDirMenu;
	os::Menu*			m_pcRootMenu;
	os::Menu*			m_pcDiskMenu;
    os::Menu*			m_pcFileMenu;
    os::Menu*			m_pcTrashMenu;
    os::RegistrarManager* m_pcManager;
    int					m_nJobsPending;
    os::Catalog*		m_pcCatalog;
};

os::BitmapImage* IconDirectoryView::Private::GetDriveIcon( os::String zPath, fs_info* psInfo, bool bSmall ) const
{
	
	os::BitmapImage* pcItemIcon = NULL;
	os::Volumes cVolumes;
	os::String cMountPoint;
	
	for( uint i = 0; i < cVolumes.GetMountPointCount(); i++ )
	{
		if( cVolumes.GetMountPoint( i, &cMountPoint ) == 0 ) {
			if( zPath == cMountPoint )
			{
				fs_info sInfo;
				if( cVolumes.GetFSInfo( i, &sInfo ) == 0 && ( sInfo.fi_flags & FS_IS_BLOCKBASED ) )
				{
					if( psInfo != NULL )
						*psInfo = sInfo;
					//std::cout<<os::Path( sInfo.fi_device_path ).GetDir().GetLeaf()<<std::endl;
					os::String zDevice = os::Path( sInfo.fi_device_path ).GetDir().GetLeaf();
					if( zDevice.Length() > 2 )
					{
						os::StreamableIO* pcSource = NULL;
						if( zDevice[0] == 'c' && zDevice[1] == 'd' ) {
							try {
								pcSource = new os::File( "/system/icons/cdrom.png" );
							} catch( ... ) {
								pcSource = new MemFile( g_aCDImage, sizeof( g_aCDImage ) );
							}
						}
						else if( zDevice[0] == 'f' && zDevice[1] == 'd' ) {
							try {
								pcSource = new os::File( "/system/icons/floppy.png" );
							} catch( ... ) {
								pcSource = new MemFile( g_aFloppyImage, sizeof( g_aFloppyImage ) );
							}
						}
						else {
							try {
								pcSource = new os::File( "/system/icons/harddisk.png" );
							} catch( ... ) {
								pcSource = new MemFile( g_aDiskImage, sizeof( g_aDiskImage ) );
							}
						}
						
						pcItemIcon = new os::BitmapImage();
									
						pcItemIcon->Load( pcSource );
						delete( pcSource );
						
					}
				}
			}
		}
	}
	if( pcItemIcon == NULL )
	{
		/* Default icon */
		MemFile* pcSource = new MemFile( g_aDiskImage, sizeof( g_aDiskImage ) );
		pcItemIcon = new os::BitmapImage();
		pcItemIcon->Load( pcSource );
		delete( pcSource );
	}
	
	if( bSmall ) {
		if( pcItemIcon && !( pcItemIcon->GetSize() == os::Point( 24, 24 ) ) )
			pcItemIcon->SetSize( os::Point( 24, 24 ) );
	} else {
		if( pcItemIcon && !( pcItemIcon->GetSize() == os::Point( 48, 48 ) ) )
			pcItemIcon->SetSize( os::Point( 48, 48 ) );
	}
	
	return( pcItemIcon );
}


/** Creates a new directory view.
 * \par Description:
 * Creates a new directory view.
 * \param cFrame - Frame.
 * \param cPath - The start path. Can be set afterwards with SetPath().
 * \param nResizeMask - ResizeMask.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
IconDirectoryView::IconDirectoryView( const Rect & cFrame, const String& cPath, uint32 nResizeMask ):IconView( cFrame, "_icon_view", nResizeMask )
{
	m = new IconDirectoryView::Private();
	
	m->m_pcCatalog = Application::GetInstance()->GetApplicationLocale()->GetLocalizedSystemCatalog( "libsyllable.catalog" );
	if( !m->m_pcCatalog ) {
		m->m_pcCatalog = new Catalog();
	}
	
	m->m_cPath = cPath;
	m->m_pcDirChangeMsg = NULL;
	m->m_pcDirKeeper = NULL;
	m->m_bLocked = false;
	m->m_bAutoLaunch = true;
	m->m_pcDirMenu = m->m_pcFileMenu = m->m_pcDiskMenu = m->m_pcRootMenu = m->m_pcTrashMenu = NULL;
	m->m_pcManager = NULL;
	
	try
	{
		m->m_pcManager = RegistrarManager::Get();
	} catch( ... ) {}
	
	/* Get default single-click state */
	os::Event cEvent;
	if( cEvent.SetToRemote( "os/Desktop/SetSingleClickInterface" ) == 0 )
	{
		os::Message cMsg;
		if( cEvent.GetLastEventMessage( &cMsg ) == 0 )
		{
			bool bSingleClick = false;
			cMsg.FindBool( "single_click", &bSingleClick );
			SetSingleClick( bSingleClick );
		}
	}
	
	m->m_nJobsPending = 0;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

IconDirectoryView::~IconDirectoryView()
{
	delete( m->m_pcDirMenu );
	delete( m->m_pcFileMenu );
	delete( m->m_pcRootMenu );
	delete( m->m_pcDiskMenu );
	delete( m->m_pcTrashMenu );
	if( m->m_pcManager )
		m->m_pcManager->Put();
	if( m->m_pcCatalog )
		m->m_pcCatalog->Release();
	delete( m );
}

void IconDirectoryView::AttachedToWindow()
{
	
	m->m_pcDirKeeper = new DirKeeper( Messenger( this ), m->m_cPath.GetPath() );
	m->m_pcDirKeeper->Run();
	IconView::AttachedToWindow();
}

void IconDirectoryView::DetachedFromWindow()
{
	m->m_pcDirKeeper->PostMessage( os::M_QUIT );
	m->m_pcDirKeeper = NULL;
	IconView::DetachedFromWindow();
}


/** Returns whether there are still pending jobs.
 * \par Description:
 * Returns whether there are still pending jobs like copying, deleting, etc...
 * The view should not be deleted before false is returned.
 * \return Whether there are pending jobs.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
bool IconDirectoryView::JobsPending() const
{
	//std::cout<<m_nJobsPending<<" pending"<<std::endl;
	return( m->m_nJobsPending > 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconDirectoryView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	switch ( pzString[0] )
	{
		case VK_DELETE:
		{
			if( GetWindow() )
				GetWindow()->PostMessage( M_DELETE, this );
			break;
		}
		case VK_BACKSPACE:
			if( m->m_bLocked )
			return( IconView::KeyDown( pzString, pzRawString, nQualifiers ) );
			m->m_cPath.Append( ".." );
			ReRead();
			DirChanged( m->m_cPath.GetPath() );
		break;
		case VK_FUNCTION_KEY:
		{
			Looper *pcLooper = GetLooper();

			assert( pcLooper != NULL );
			Message *pcMsg = pcLooper->GetCurrentMessage();

			assert( pcMsg != NULL );
			int32 nKeyCode;

			if( pcMsg->FindInt32( "_raw_key", &nKeyCode ) != 0 )
			{
				return;
			}
			switch ( nKeyCode )
			{
			case 6:	// F5
				ReRead();
				break;
			}
			break;
		}
		default:
			IconView::KeyDown( pzString, pzRawString, nQualifiers );
		break;
	}
}


/** Rereads the directory content.
 * \par Description:
 * Rereads the directory content. Necessary after adding the view to a window
 * or after a SetPath() call.
 * \par Note:
 * Changes in the directory will automatically cause an update.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconDirectoryView::ReRead()
{
	//std::cout<<"REREAD"<<std::endl;
	
	if( m->m_pcDirKeeper )
	{
		m->m_pcDirKeeper->Lock();
		m->m_pcDirKeeper->Stop();
	}
	Clear();
	/* Make sure we delete all pending messages from the dirkeeper thread */
	if( GetLooper() )
	{
		os::Looper* pcLooper = GetLooper();
		pcLooper->SpoolMessages();	// Copy all messages from the message port to the message queue.
		MessageQueue *pcQueue = pcLooper->GetMessageQueue();
		MessageQueue cTmp;
		Message *pcMsg;

		pcQueue->Lock();

		while( ( pcMsg = pcQueue->NextMessage() ) != NULL )
		{
			switch( pcMsg->GetCode() )
			{
				case M_ADD_ENTRY:
				case M_UPDATE_ENTRY:
				case M_REMOVE_ENTRY:
				case M_LAYOUT:
				case M_NEW_DIR:
				case M_DELETE:
				case M_RENAME:
				case M_OPEN_WITH:
				case M_INFO:
				case M_MOVE_TO_TRASH:
				case M_EMPTY_TRASH:
				case M_REREAD:
					break;
				default:
					cTmp.AddMessage( pcMsg );
			}
		}
		
		while( ( pcMsg = cTmp.NextMessage() ) != NULL )
		{
			pcQueue->AddMessage( pcMsg );
		}
		pcQueue->Unlock();
	}
		
	Message cMsg( DirKeeper::M_CHANGE_DIR );

	cMsg.AddString( "path", m->m_cPath.GetPath() );
	if( m->m_pcDirKeeper )
	{
		m->m_pcDirKeeper->PostMessage( &cMsg, m->m_pcDirKeeper );
		m->m_pcDirKeeper->Unlock();
	}

}



/** Returns the current path.
 * \par Description:
 * Returns the current path.
 * \return The current path.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::String IconDirectoryView::GetPath() const
{
	return ( m->m_cPath.GetPath() );
}


/** Sets the current path.
 * \par Description:
 * Sets the current path. After this call you have to call the ReRead() method.
 * \param cPath - The new path. Please make sure that is is valid.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconDirectoryView::SetPath( const String& cPath )
{
	m->m_cPath.SetTo( cPath.c_str() );
	DirChanged( m->m_cPath.GetPath() );
}


/** Returns the icon of the current directory.
 * \par Description:
 * Returns the icon of the current directory. The iconsize is 24x24 pixels.
 * \return The icon.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
os::Image* IconDirectoryView::GetDirIcon() const
{
	/* Look if the path is a mountpoint */
	os::Volumes cVolumes;
	os::String cMountPoint;
	for( uint i = 0; i < cVolumes.GetMountPointCount(); i++ )
	{
		if( cVolumes.GetMountPoint( i, &cMountPoint ) == 0 ) {
			if( m->m_cPath.GetPath() == cMountPoint )
				return( m->GetDriveIcon( m->m_cPath.GetPath(), NULL, GetView() != VIEW_ICONS 
						&& GetView() != VIEW_ICONS_DESKTOP ) );
		}
	}
	os::String zTempType;
	os::Image* pcIcon;
	
	if( m->m_pcManager && m->m_pcManager->GetTypeAndIcon( m->m_cPath.GetPath(), os::Point( 24, 24 ),
									&zTempType, &pcIcon ) == 0)
	{		
	} else {
		os::BitmapImage* pcBitmap = new os::BitmapImage();
		
		MemFile* pcSource = new MemFile( g_aFolderImage, sizeof( g_aFolderImage ) );
							
		pcBitmap->Load( pcSource );
		delete( pcSource );
		pcIcon = pcBitmap;
	}
	return( pcIcon );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconDirectoryView::Invoked( uint nIcon, os::IconData* pcIconData )
{
	//std::cout<<"Invoked "<<GetIconString( nIcon, 0 ).c_str()<<std::endl;
	
	if( nIcon >= GetIconCount() )
	{
		IconView::Invoked( nIcon, pcIconData );
		return;
	}
	
	DirectoryIconData *pcData = static_cast < DirectoryIconData * >( pcIconData );
	
	if( pcData->m_bBrokenLink )
	{
		IconView::Invoked( nIcon, pcIconData );
		return;
	}
	
	//std::cout<<pcData->m_sStat.st_mode<<std::endl;
	
	if( S_ISDIR( pcData->m_sStat.st_mode ) == false )
	{
		/* Check if it is a executable */
		if( m->m_pcManager && m->m_bAutoLaunch )
		{
			os::Path cPath( m->m_cPath );
			cPath.Append( pcData->m_zPath );
			m->m_pcManager->Launch( GetWindow(), cPath, true, m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_WINDOW_OPEN_WITH, "Open with..." ) );
		} else if( m->m_bAutoLaunch ) {
			if( pcData->m_sStat.st_mode & ( S_IXUSR|S_IXGRP|S_IXOTH ) ) {
				if( fork() == 0 )
				{
					os::Path cPath( m->m_cPath );
					cPath.Append( pcData->m_zPath.c_str() );
					set_thread_priority( -1, 0 );
					execlp( cPath.GetPath().c_str(), cPath.GetPath().c_str(), NULL, NULL );
				}
			}
		}
		
		
		IconView::Invoked( nIcon, pcIconData );
		return;
	}
	
	/* Check symlinks */
	char zNewPath[PATH_MAX];
	os::Path cPath = m->m_cPath;
	cPath.Append( pcData->m_zPath.c_str() );
	os::FSNode cNode( cPath.GetPath(), O_RDONLY | O_NONBLOCK | O_NOTRAVERSE );
	
	if( cNode.IsLink() )
	{
		/* Read link */
		os::SymLink cLink( cPath.GetPath(), O_RDONLY );
		cLink.ConstructPath( m->m_cPath.GetPath(), &cPath );
		strcpy( zNewPath, cPath.GetPath().c_str() );
	} else {
		strcpy( zNewPath, cPath.GetPath().c_str() );
	}
	
	if( m->m_bLocked )
	{
		/* Just post message */
		DirChanged( zNewPath );
		return;
	}
	
	/* Change directory */
	m->m_cPath = zNewPath;
	ReRead();

	DirChanged( m->m_cPath.GetPath() );
}


/** Sets the message that is sent when the directory changes.
 * \par Description:
 * Sets the message that is sent when the directory changes.
 * \param pcMsg - The message.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconDirectoryView::SetDirChangeMsg( Message * pcMsg )
{
	delete m->m_pcDirChangeMsg;

	m->m_pcDirChangeMsg = pcMsg;
}

/** Sets whether the current directory is locked.
 * \par Description:
 * Sets whether the current directory is locked. When the user tries to access
 * another directory, the message set by SetDirChangeMsg() is sent, but the 
 * directory is not changed.
 * \param bLocked - Enables/Disables this feature.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconDirectoryView::SetDirectoryLocked( bool bLocked )
{
	m->m_bLocked = bLocked;
}


/** Sets whether invoked files are automatically launched.
 * \par Description:
 * Sets whether invoked files are automatically launched when they are invoked.
 * \param bAutoLaunch - Enables/Disables this feature.
 * \author	Arno Klenke (arno_klenke@yahoo.de)
 *****************************************************************************/
void IconDirectoryView::SetAutoLaunch( bool bAutoLaunch )
{
	m->m_bAutoLaunch = bAutoLaunch;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconDirectoryView::DirChanged( const String& cNewPath )
{
	if( m->m_pcDirChangeMsg != NULL )
	{
		os::Message cMsg( *m->m_pcDirChangeMsg );
		cMsg.AddString( "file/path", cNewPath );
		Invoke( &cMsg );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconDirectoryView::DragSelection( os::Point cStartPoint )
{
	Message cData( 1234 );
	int nNumObjects = 0;
	int nLastSelected = 0;
	os::Point cIconPos = cStartPoint;
	
	/* Collect selected icons */
	for( uint i = 0; i < GetIconCount(); i++ )
	{
		if( GetIconSelected( i ) )
		{
			
			DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );
			Path cPath = m->m_cPath;

			cPath.Append( pcData->m_zPath.c_str() );
			cData.AddString( "file/path", cPath.GetPath() );
			nNumObjects++;
			
			if( os::Rect( GetIconPosition( i ), GetIconPosition( i ) + GetIconSize() ).DoIntersect(
				cStartPoint ) )
				cIconPos = GetIconPosition( i );
			
			nLastSelected = i;
			//std::cout<<cPath.GetPath()<<std::endl;
		}
	}
	cData.AddPoint( "drag/start", cStartPoint );
	
	/* Create drag object */
	os::Bitmap cImage( (int)GetIconSize().x + 1, (int)GetIconSize().y + 1, os::CS_RGB32, os::Bitmap::ACCEPT_VIEWS | os::Bitmap::SHARE_FRAMEBUFFER );
	View *pcView = new View( os::Rect( os::Point( 0, 0 ), GetIconSize() ), "" );
	cImage.AddChild( pcView );
	
	os::Image* pcIcon = NULL;		
	
	char zString[255];
	if( nNumObjects == 1 && GetIconImage( nLastSelected ) )
	{
		/* One file/directory */
		strcpy( zString, GetIconString( nLastSelected, 0 ).c_str() );
		pcIcon = GetIconImage( nLastSelected );
	} else {
		/* Some objects */
		os::BitmapImage* pcBitmapIcon = new os::BitmapImage();
		sprintf( zString, m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_NUMBER_OF_OBJECTS_SELECTED, "%i Entries" ).c_str(), nNumObjects );
		MemFile* pcSource = new MemFile( g_aFolderImage, sizeof( g_aFolderImage ) );
		pcBitmapIcon->Load( pcSource );
		if( GetView() == VIEW_LIST || GetView() == VIEW_DETAILS )
			pcBitmapIcon->SetSize( os::Point( 24, 24 ) );
		pcIcon = pcBitmapIcon;
		delete( pcSource );
	}
	if( pcIcon )
		RenderIcon( zString, pcIcon, pcView, os::Point( 0, 0 ) );
	cImage.Sync();
	if( nNumObjects != 1 )
		delete( pcIcon );
	
	BeginDrag( &cData, cStartPoint - cIconPos, &cImage/*os::Rect( os::Point(), GetIconSize() )*/ );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconDirectoryView::OpenContextMenu( os::Point cPosition, bool bMouseOverIcon )
{
	/* Create context menu */
	
	if( !bMouseOverIcon )
	{
		
		if( m->m_cPath.GetPath() == "/" )
		{
			//std::cout<<"Root menu!"<<std::endl;
			if( !m->m_pcRootMenu ) {
				m->m_pcRootMenu = new os::Menu( os::Rect(), "rootmenu", os::ITEMS_IN_COLUMN );
				m->m_pcRootMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_MOUNT, "Mount..." ), new os::Message( M_MOUNT ) );
				m->m_pcRootMenu->SetTargetForItems( this );
			}
			m->m_pcRootMenu->Open( ConvertToScreen( cPosition ) );
		} else {
			/* Directory menu */
			//std::cout<<"Directory menu!"<<std::endl;
			if( !m->m_pcDirMenu ) {
				m->m_pcDirMenu = new os::Menu( os::Rect(), "dirmenu", os::ITEMS_IN_COLUMN );
				m->m_pcDirMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_NEW_DIRECTORY, "New directory..." ), new os::Message( M_NEW_DIR ) );
				m->m_pcDirMenu->SetTargetForItems( this );
			}
			m->m_pcDirMenu->Open( ConvertToScreen( cPosition ) );
		}
	} else if( !( m->m_cPath.GetPath() == "/" ) )
	{
		/* File or Trash menu */
		int nSelectCount = 0;
		bool bHaveTrash = false;
		bool bIsTrash = false;
		
		/* Get the inode number of the trash folder */
		struct stat sTrashStat;
		os::Path cTrash( getenv( "HOME" ) );
		cTrash.Append( "Trash" );
		if( lstat( cTrash.GetPath().c_str(), &sTrashStat ) >= 0 )
			bHaveTrash = true;
			
		for( uint i = 0; i < GetIconCount(); i++ ) {
			if( GetIconSelected( i ) )
			{
				/* Check if the selected item is the trash folder */
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );
				if( bHaveTrash && pcData->m_sStat.st_dev == sTrashStat.st_dev && 
					pcData->m_sStat.st_ino == sTrashStat.st_ino ) {
					bIsTrash = true;
					break;
				}
				nSelectCount++;
			}
		}	
		//std::cout<<nSelectCount<<std::endl;
		
		if( bIsTrash )
		{
			if( !m->m_pcTrashMenu )
			{
				m->m_pcTrashMenu = new os::Menu( os::Rect(), "trashmenu", os::ITEMS_IN_COLUMN );
				m->m_pcTrashMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_EMPTY_TRASH, "Empty" ), new os::Message( M_EMPTY_TRASH ) );		
				m->m_pcTrashMenu->SetTargetForItems( this );
			}
			m->m_pcTrashMenu->Open( ConvertToScreen( cPosition ) );
			
			return;
		}
				
		if( !m->m_pcFileMenu ) {
			m->m_pcFileMenu = new os::Menu( os::Rect(), "filemenu", os::ITEMS_IN_COLUMN );
			
			m->m_pcFileMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_OPEN_WITH, "Open with..." ), new os::Message( M_OPEN_WITH ) );
			m->m_pcFileMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_INFO, "Info..." ), new os::Message( M_INFO ) );
			m->m_pcFileMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_RENAME, "Rename..." ), new os::Message( M_RENAME ) );
			m->m_pcFileMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_MOVE_TO_TRASH, "Move to Trash" ), new os::Message( M_MOVE_TO_TRASH ) );
			m->m_pcFileMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_DELETE, "Delete..." ), new os::Message( M_DELETE ) );
			m->m_pcFileMenu->SetTargetForItems( this );
		}
		
		m->m_pcFileMenu->GetItemAt( 0 )->SetEnable( nSelectCount == 1 );
		m->m_pcFileMenu->GetItemAt( 1 )->SetEnable( nSelectCount == 1 && m->m_pcManager );
		m->m_pcFileMenu->GetItemAt( 2 )->SetEnable( nSelectCount == 1 );
		m->m_pcFileMenu->GetItemAt( 3 )->SetEnable( bHaveTrash );
		
		m->m_pcFileMenu->Open( ConvertToScreen( cPosition ) );
		
	} else {
		/* Drive menu */
		if( !m->m_pcDiskMenu ) {
			m->m_pcDiskMenu = new os::Menu( os::Rect(), "diskmenu", os::ITEMS_IN_COLUMN );
			
			m->m_pcDiskMenu->AddItem( m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_MENU_UNMOUNT, "Unmount" ), new os::Message( M_UNMOUNT ) );
			m->m_pcDiskMenu->SetTargetForItems( this );
		}
		
		m->m_pcDiskMenu->Open( ConvertToScreen( cPosition ) );
	}
	//std::cout<<"Context menu"<<std::endl;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconDirectoryView::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case M_ADD_ENTRY:
		{
			const char *pzName;
			const char* pzPath;
			const struct stat * psStat;
			bool bBrokenLink;
			size_t nSize;
			os::NodeMonitor* pcMonitor;

			
				if( pcMessage->FindString( "name", &pzName ) != 0 )
				{
					break;
				}
				if( pcMessage->FindString( "path", &pzPath ) != 0 )
				{
					break;
				}
				if( pcMessage->FindData( "stat", T_ANY_TYPE, ( const void ** )&psStat, &nSize ) != 0 )
				{
					break;
				}
				if( pcMessage->FindBool( "broken_link", &bBrokenLink ) != 0 )
				{
					break;
				}
				if( pcMessage->FindPointer( "node_monitor", (void**)&pcMonitor ) != 0 )
				{
					break;
				}
				
				try
				{
					os::Path cPath( m->m_cPath );
					cPath.Append( pzPath );
					os::Image* pcImage = NULL;
					fs_info sInfo;
					os::String zType( "" );
					
					if( m->m_cPath.GetPath() == "/" )
						pcImage = m->GetDriveIcon( cPath.GetPath(), &sInfo, GetView() != VIEW_ICONS 
							&& GetView() != VIEW_ICONS_DESKTOP);
					else {
						os::Point cRequestedSize;
						if( GetView() != VIEW_ICONS && GetView() != VIEW_ICONS_DESKTOP )
							cRequestedSize = os::Point( 24, 24 );
						else
							cRequestedSize = os::Point( 48, 48 );
						if( m->m_pcManager && m->m_pcManager->GetTypeAndIcon( cPath.GetPath(), cRequestedSize,
							&zType, &pcImage ) == 0)
						{
						} 
						else
						{
							os::BitmapImage* pcBitmap = new os::BitmapImage();
							MemFile* pcSource;
							
							if( S_ISDIR( psStat->st_mode ) )
								pcSource = new MemFile( g_aFolderImage, sizeof( g_aFolderImage ) );
							else
								pcSource = new MemFile( g_aFileImage, sizeof( g_aFileImage ) );
							pcBitmap->Load( pcSource );
							delete( pcSource );
							if( pcBitmap->GetSize() != cRequestedSize )
								pcBitmap->SetSize( cRequestedSize );
							pcImage = pcBitmap;
						}
					}
					
					char zSize[255];
					if( m->m_cPath.GetPath() == "/" )
						sprintf( zSize, m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_PERCENT_FREE, "%i%% free" ).c_str(),
							( int )( sInfo.fi_free_blocks * 100 / sInfo.fi_total_blocks ) );
					else if( psStat->st_size >= 1000000 )
						sprintf( zSize, "%i Mb", ( int )( psStat->st_size / 1000000 + 1 ) );
					else
						sprintf( zSize, "%i Kb", ( int )( psStat->st_size / 1000 + 1 ) );
					DirectoryIconData* pcData = new DirectoryIconData;
					pcData->m_zPath = pzPath;
					pcData->m_sStat = *psStat;
					pcData->m_bBrokenLink = bBrokenLink;
					pcData->m_pcMonitor = pcMonitor;
					
					uint nIcon = AddIcon( pcImage, pcData );
					AddIconString( nIcon, pzName );
					if( !S_ISDIR( psStat->st_mode ) || ( m->m_cPath.GetPath() == "/" ) )
						AddIconString( nIcon, zSize );
					else
						AddIconString( nIcon, "" );
					AddIconString( nIcon, zType );		
				}
				catch( std::exception & )
				{
				}
			
			if( m->m_pcDirKeeper )
				m->m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_ADDED, m->m_pcDirKeeper, m->m_pcDirKeeper );
			break;
		}
	case M_UPDATE_ENTRY:
		{
			const char *pzName;
			const char* pzPath;
			const struct stat * psStat;
			struct stat sNewStat;
			size_t nSize;
			long long nNodeSize;
//			int nCount = 0;
			bool bBrokenLink;
			bool bReloadIcon;
			
//			pcMessage->GetNameInfo( "stat", NULL, &nCount );
//			for( int i = 0; i < nCount; ++i )
//			{
				if( pcMessage->FindString( "name", &pzName ) != 0 )
				{
					break;
				}
				if( pcMessage->FindString( "path", &pzPath ) != 0 )
				{
					break;
				}
				if( pcMessage->FindData( "stat", T_ANY_TYPE, ( const void ** )&psStat, &nSize ) != 0 )
				{
					break;
				}
				if( pcMessage->FindBool( "broken_link", &bBrokenLink ) != 0 )
				{
					break;
				}
				if( pcMessage->FindBool( "reload_icon", &bReloadIcon ) != 0 )
				{
					break;
				}
				try
				{
					//std::cout<<"UPDATE "<<bReloadIcon<<std::endl;
					/* Check if we need to remove an old entry */
					for( uint j = 0; j < GetIconCount(); j++ )
					{
						DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( j ) );
	
						if( GetIconString( j, 0 ) == pzName )
						{
							RemoveIcon( j );
							break;
						}
					}
					
					for( uint j = 0; j < GetIconCount(); j++ )
					{
						DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( j ) );
	
						//std::cout<<pcData->m_sStat.st_ino<<" "<<psStat->st_ino<<" "<<pcData->m_sStat.st_dev<<" "<<psStat->st_dev<<std::endl;
	
						if( pcData->m_sStat.st_ino == psStat->st_ino && pcData->m_sStat.st_dev == psStat->st_dev )
						{
							os::Path cPath( m->m_cPath );
							cPath.Append( pzPath );
							os::Image* pcImage = NULL;
							fs_info sInfo;
							os::String zType( "" );
							
							/* Update the stat information if possible */
							os::FSNode cNode;
							if( m->m_pcDirKeeper->GetNode( GetIconString( j, 0 ), &cNode, &bBrokenLink ) == 0 )
							{
								cNode.GetStat( &sNewStat, false );	
								nNodeSize = sNewStat.st_size;
								
								//std::cout<<"NEW "<<sNewStat.st_ino<<" "<<sNewStat.st_mode<<std::endl;
							} else {
								nNodeSize = psStat->st_size;
								sNewStat = *psStat;
							}
							
							
							/* Update Icon if requested */
							if( !( m->m_cPath.GetPath() == "/" ) && bReloadIcon )
							{
								
								os::Point cRequestedSize;
								if( GetView() != VIEW_ICONS && GetView() != VIEW_ICONS_DESKTOP )
									cRequestedSize = os::Point( 24, 24 );
								else
									cRequestedSize = os::Point( 48, 48 );
								if( m->m_pcManager && m->m_pcManager->GetTypeAndIcon( cPath.GetPath(), cRequestedSize,
									&zType, &pcImage ) == 0 )
								{
									
								} 
								else
								{
									os::BitmapImage* pcBitmap = new os::BitmapImage();
									MemFile* pcSource;
							
									if( S_ISDIR( psStat->st_mode ) )
										pcSource = new MemFile( g_aFolderImage, sizeof( g_aFolderImage ) );
									else
										pcSource = new MemFile( g_aFileImage, sizeof( g_aFileImage ) );
									pcBitmap->Load( pcSource );
									delete( pcSource );
									pcImage = pcBitmap;
								}
								SetIconImage( j, pcImage );
							} 
							
							/* Set a new name if one is provided */
							if( strcmp( pzName, "" ) != 0 )
								SetIconString( j, 0, os::String( pzName ) );
								
							char zSize[255];
							if( m->m_cPath.GetPath() == "/" ) {
								//sprintf( zSize, m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_PERCENT_FREE, "%i%% free" ).c_str(), ( int )( sInfo.fi_free_blocks * 100 / sInfo.fi_total_blocks ) );
							}
							else if( nNodeSize >= 1000000 )
								sprintf( zSize, "%i Mb", ( int )( nNodeSize / 1000000 + 1 ) );
							else
								sprintf( zSize, "%i Kb", ( int )( nNodeSize / 1000 + 1 ) );
							if( !S_ISDIR( sNewStat.st_mode ) )
								SetIconString( j, 1, zSize );
							
							if( strcmp( pzPath, "" ) != 0 )
								pcData->m_zPath = pzPath;
							pcData->m_sStat = sNewStat;
							pcData->m_bBrokenLink = bBrokenLink;
							break;
						}
					}
				}
				catch( std::exception & )
				{
				}
//			}
			if( m->m_pcDirKeeper )
				m->m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_UPDATED, m->m_pcDirKeeper, m->m_pcDirKeeper );
			
			break;
		}
	case M_REMOVE_ENTRY:
		{
			dev_t nDevice;
			int64 nInode;
			bool bFound = false;

//			int nCount = 0;

	//		pcMessage->GetNameInfo( "device", NULL, &nCount );

//			for( int i = 0; i < nCount; ++i )
//			{
				pcMessage->FindInt( "device", &nDevice );
				pcMessage->FindInt64( "node", &nInode );

				uint j;
				for( j = 0; j < GetIconCount(); j++ )
				{
					DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( j ) );
	
					if( pcData->m_sStat.st_ino == nInode && (dev_t)pcData->m_sStat.st_dev == nDevice )
					{
						bFound = true;
						RemoveIcon( j );
						break;
					}
				}
//			}
			
			if( m->m_pcDirKeeper )
				m->m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_REMOVED, m->m_pcDirKeeper, m->m_pcDirKeeper );
			if( !bFound )
			{
				ReRead();
			}
			break;
		}
	case M_LAYOUT:
	{
//		bigtime_t nTime = get_system_time();
		Layout();
//		printf( "Relayout took %i\n", (int)( get_system_time() - nTime ) );
		if( m->m_pcDirKeeper )
			m->m_pcDirKeeper->PostMessage( DirKeeper::M_LAYOUT_UPDATED, m->m_pcDirKeeper, m->m_pcDirKeeper );
		break;
	}
	case M_MOUNT:
	{
		Volumes cVolumes;
		os::Window* pcDialog = cVolumes.MountDialog( this, new os::Message( M_REREAD ) );
		if( GetWindow() )
			pcDialog->CenterInWindow( GetWindow() );
		pcDialog->Show();
		pcDialog->MakeFocus();
		break;
	}
	case M_UNMOUNT:
	{
		std::vector < std::string > cPaths;
		for( uint i = 0; i < GetIconCount(); ++i )
		{
			if( GetIconSelected( i ) )
			{
				Path cPath = m->m_cPath;
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );

				cPath.Append( pcData->m_zPath.c_str() );
				//printf( "Unmount %s\n", cPath.GetPath().c_str() );
				m->m_nJobsPending++;
				Volumes cVolumes;
				cVolumes.Unmount( cPath.GetPath(), this, new os::Message( M_JOB_END_REREAD ) );
			}
		}
		break;
	}
	case M_NEW_DIR:
	{
		if( !( m->m_cPath.GetPath() == "/" ) )
		{
			os::Window* pcDialog;
			if( m->m_pcDirKeeper )
			{
				pcDialog = m->m_pcDirKeeper->GetCurrentDir()->CreateDirectoryDialog( this, NULL/*new os::Message( M_REREAD )*/, "Directory name" );
				if( GetWindow() )
					pcDialog->CenterInWindow( GetWindow() );
				pcDialog->Show();
				pcDialog->MakeFocus();
			}
		}
		break;
	}
	case M_DELETE:
	{
		std::vector < os::String > cPaths;
		for( uint i = 0; i < GetIconCount(); ++i )
		{
			if( GetIconSelected( i ) )
			{
				Path cPath = m->m_cPath;
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );

				cPath.Append( pcData->m_zPath.c_str() );
				cPaths.push_back( cPath.GetPath() );
			}
		}
		if( !( m->m_cPath.GetPath() == "/" ) && cPaths.size() ) {
			m->m_nJobsPending++;
			delete_files( cPaths, Messenger( this ), new os::Message( M_JOB_END ) );
		}
		break;
	}
	case M_RENAME:
	{
		for( uint i = 0; i < GetIconCount(); ++i )
		{
			if( GetIconSelected( i ) )
			{
				Path cPath = m->m_cPath;
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );

				cPath.Append( pcData->m_zPath.c_str() );
				if( !( m->m_cPath.GetPath() == "/" ) )
				{
					os::Window* pcDialog;
					try
					{
						FileReference cRef( cPath.GetPath() );
						
						pcDialog = cRef.RenameDialog( this, NULL/*new os::Message( M_REREAD )*/ );
						if( GetWindow() )
							pcDialog->CenterInWindow( GetWindow() );
						pcDialog->Show();
						pcDialog->MakeFocus();
					} catch( ... )
					{
					}
				}
				break;
			}
		}
		break;
	}
	case M_OPEN_WITH:
	{
		for( uint i = 0; i < GetIconCount(); ++i )
		{
			if( GetIconSelected( i ) )
			{
				Path cPath = m->m_cPath;
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );

				cPath.Append( pcData->m_zPath.c_str() );
				if( !( m->m_cPath.GetPath() == "/" ) && m->m_pcManager )
				{
					m->m_pcManager->Launch( GetWindow(), cPath, true, m->m_pcCatalog->GetString( ID_MSG_ICONDIRVIEW_WINDOW_OPEN_WITH, "Open with..." ), false );
				}
				break;
			}
		}
		break;
	}
	case M_INFO:
	{
		for( uint i = 0; i < GetIconCount(); ++i )
		{
			if( GetIconSelected( i ) )
			{
				Path cPath = m->m_cPath;
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );

				cPath.Append( pcData->m_zPath.c_str() );
				if( !( m->m_cPath.GetPath() == "/" ) && m->m_pcManager )
				{
					os::Window* pcDialog;
					try
					{
						FileReference cRef( cPath.GetPath() );
						pcDialog = cRef.InfoDialog( this, new os::Message( M_REREAD ) );
						if( GetWindow() )
							pcDialog->CenterInWindow( GetWindow() );
						pcDialog->Show();
						pcDialog->MakeFocus();
					} catch( ... )
					{
					}
				}
				break;
			}
		}
		break;
	}
	case M_MOVE_TO_TRASH:
	{
		std::vector < os::String > cSrcPaths;
		std::vector < os::String > cDstPaths;
		for( uint i = 0; i < GetIconCount(); ++i )
		{
			if( GetIconSelected( i ) )
			{
				Path cSrcPath = m->m_cPath;
				Path cDstPath( getenv( "HOME" ) );
				cDstPath.Append( "Trash" );
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );

				cSrcPath.Append( pcData->m_zPath.c_str() );
				cDstPath.Append( pcData->m_zPath.c_str() );
				
				cSrcPaths.push_back( cSrcPath.GetPath() );
				cDstPaths.push_back( cDstPath.GetPath() );
			}
		}
		if( !( m->m_cPath.GetPath() == "/" ) && cSrcPaths.size() ) {
			/* Start file move */
			m->m_nJobsPending++;
			move_files( cDstPaths, cSrcPaths, Messenger( this ), new os::Message( M_JOB_END ) );
		}
		break;
	}
	case M_EMPTY_TRASH:
	{
		std::vector < os::String > cPaths;
		Path cTrashPath( getenv( "HOME" ) );
		cTrashPath.Append( "Trash" );
		Directory* pcTrashDir = new Directory( cTrashPath );
		String zEntry;
		pcTrashDir->Rewind();
		
		while( pcTrashDir->GetNextEntry( &zEntry ) == 1 )
		{
			//std::cout<<zEntry.c_str()<<std::endl;
			if( !( zEntry == "." ) && !( zEntry == ".." ) )
			{
				Path cPath( cTrashPath );
				cPath.Append( zEntry );
				cPaths.push_back( cPath.GetPath() );
			}
		}
		if( cPaths.size() ) {
			m->m_nJobsPending++;
			delete_files( cPaths, Messenger( this ), new os::Message( M_JOB_END ) );
		}
		break;
	}
	case M_JOB_END:
		m->m_nJobsPending--;
		break;
	case M_JOB_END_REREAD:
		m->m_nJobsPending--;
	case M_REREAD:
		ReRead();
		break;
	default:
		IconView::HandleMessage( pcMessage );
	}
}
//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconDirectoryView::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	if( pcData == NULL )
	{
		IconView::MouseUp( cPosition, nButtons, NULL );
		return;
	}
	StopScroll();

	const char *pzPath;

	if( pcData->FindString( "file/path", &pzPath ) != 0 )
	{
		IconView::MouseUp( cPosition, nButtons, NULL );
		return;
	}
	
	Path cDstDir;

	cDstDir = m->m_cPath;
		
	/* Check what icon is under the mouse */
	for( uint i = 0; i < GetIconCount(); i++ )
	{
		if( os::Rect( GetIconPosition( i ), GetIconPosition( i ) + GetIconSize() ).DoIntersect(
			ConvertToView( cPosition ) ) )
		{
			DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( i ) );
			if( S_ISDIR( pcData->m_sStat.st_mode ) )
			{
				//std::cout<<"Mouse "<<pcData->m_zPath.c_str()<<std::endl;
				cDstDir.Append( pcData->m_zPath.c_str() );
				break;
			}
		}
	}
	
	
	//std::cout<<pzPath<<" "<<cDstDir.GetPath().c_str()<<std::endl;
	
	if( m->m_cPath == Path( pzPath ).GetDir() && ( ( cDstDir == m->m_cPath ) || ( cDstDir == Path( pzPath ) ) ) 
		&& ( GetView() == VIEW_DETAILS || GetView() == VIEW_LIST ) )
		return( IconView::MouseUp( cPosition, nButtons, NULL ) );
	
	if( m->m_cPath == Path( pzPath ).GetDir() && ( ( cDstDir == m->m_cPath ) || ( cDstDir == Path( pzPath ) ) ) )
	{
		/* Move icon position */
		int i = 0;
		os::Point cDeltaMove;
		os::Point cStartPoint;
		
		if( pcData->FindPoint( "drag/start", &cStartPoint ) != 0 )
			return( os::IconView::MouseUp( cPosition, nButtons, NULL ) );
			
		/* Calculate move */
		cDeltaMove.x = ConvertToView( cPosition ).x - cStartPoint.x;
		cDeltaMove.y = ConvertToView( cPosition ).y - cStartPoint.y;
		
		while( pcData->FindString( "file/path", &pzPath, i ) == 0 )
		{
			/* Find in icon list */
			for( uint j = 0; j < GetIconCount(); j++ )
			{
				DirectoryIconData *pcData = static_cast < DirectoryIconData * >( GetIconData( j ) );
				Path cFilePath = m->m_cPath;
				cFilePath.Append( pcData->m_zPath );
				
				if( cFilePath == Path( pzPath ) )
				{
					/* Move icon */
					SetIconPosition( j, GetIconPosition( j ) + cDeltaMove );
				}
			}			
			i++;
		}
		return( os::IconView::MouseUp( cPosition, nButtons, NULL ) );
	} else
	{
		std::vector < os::String > cSrcPaths;
		std::vector < os::String > cDstPaths;
		for( int i = 0; pcData->FindString( "file/path", &pzPath, i ) == 0; ++i )
		{
			/* Check that none of the source items is the same as our destination */
			if( os::String( pzPath ) == cDstDir )
				return( IconView::MouseUp( cPosition, nButtons, NULL ) );
			Path cSrcPath( pzPath );
			Path cDstPath = cDstDir;

			cDstPath.Append( cSrcPath.GetLeaf() );
			cSrcPaths.push_back( pzPath );
			cDstPaths.push_back( cDstPath.GetPath() );
		}
		//std::cout<<"Copy!"<<std::endl;
		if( os::Application::GetInstance()->GetQualifiers() & os::QUAL_ALT )
		{
			/* Create links */
			for( uint i = 0; i < cSrcPaths.size(); i++ )
			{
//				printf( "Create link %s to %s\n", cDstPaths[i].c_str(), cSrcPaths[i].c_str() );
				symlink( cSrcPaths[i].c_str(), cDstPaths[i].c_str() );
			}
		} else if( os::Application::GetInstance()->GetQualifiers() & os::QUAL_CTRL )
		{
			/* Start file copy */
			m->m_nJobsPending++;
			copy_files( cDstPaths, cSrcPaths, Messenger( this ), new os::Message( M_JOB_END ) );
		}
		else {
			/* Start file move */
			m->m_nJobsPending++;
			move_files( cDstPaths, cSrcPaths, Messenger( this ), new os::Message( M_JOB_END ) );
		}
		return( os::IconView::MouseUp( cPosition, nButtons, NULL ) );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void IconDirectoryView::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	
	if( pcData == NULL )
	{
		IconView::MouseMove( cNewPos, nCode, nButtons, NULL );
		return;
	}
	if( nCode == MOUSE_OUTSIDE )
	{
		return;
	}
	if( nCode == MOUSE_EXITED )
	{
		StopScroll();
		return;
	}
	
	const char *pzPath;

	if( pcData->FindString( "file/path", &pzPath ) != 0 )
	{
		return;
	}

	Rect cBounds = GetBounds();

	if( GetView() == VIEW_ICONS || GetView() == VIEW_DETAILS )
	{
		if( cNewPos.y < cBounds.top + 20.0f )
		{
			StartScroll( SCROLL_UP );
		}
		else if( cNewPos.y > cBounds.bottom - 20.0f )
		{
			StartScroll( SCROLL_DOWN );
		}
		else
		{
			StopScroll();
		}
	}
	else if( GetView() == VIEW_LIST )
	{
		if( cNewPos.x < cBounds.left + 20.0f )
		{
			StartScroll( SCROLL_LEFT );
		}
		else if( cNewPos.x > cBounds.right - 20.0f )
		{
			StartScroll( SCROLL_RIGHT );
		}
		else
		{
			StopScroll();
		}
	}
	
	os::IconView::MouseMove( cNewPos, nCode, nButtons, pcData );
	
	/* Check what icon is under the mouse */
	
	for( uint i = 0; i < GetIconCount(); i++ )
	{
		if( os::Rect( GetIconPosition( i ), GetIconPosition( i ) + GetIconSize() ).DoIntersect(
				ConvertToView( cNewPos ) ) )
		{
			DirectoryIconData *pcIconData = static_cast < DirectoryIconData * >( GetIconData( i ) );
			if( S_ISDIR( pcIconData->m_sStat.st_mode ) )
			{
				Path cRowPath = m->m_cPath;

				cRowPath.Append( pcIconData->m_zPath.c_str() );
				os::String cSrcPath;
				for( int j = 0; pcData->FindString( "file/path", &cSrcPath, j) == 0; ++j )
				{
					if( cSrcPath == cRowPath )
						return;
				}
				SetIconSelected( i, true );
				return;
			}
			break;
		}
	}
}

void IconDirectoryView::__IDV_reserved1__() {}
void IconDirectoryView::__IDV_reserved2__() {}
void IconDirectoryView::__IDV_reserved3__() {}
void IconDirectoryView::__IDV_reserved4__() {}
void IconDirectoryView::__IDV_reserved5__() {}
