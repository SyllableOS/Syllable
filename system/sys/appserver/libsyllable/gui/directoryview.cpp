
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 1999 - 2001  Kurt Skauen
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
#include <sys/stat.h>
#include <assert.h>

#include <atheos/time.h>

#include <gui/directoryview.h>
#include <gui/window.h>
#include <gui/bitmap.h>
#include <gui/stringview.h>
#include <gui/button.h>
#include <gui/tableview.h>
#include <gui/requesters.h>
#include <gui/icon.h>
#include <gui/font.h>
#include <util/looper.h>
#include <util/message.h>
#include <storage/directory.h>
#include <storage/nodemonitor.h>

#include <list>
#include <set>
#include <strstream>
//#include <sstream>

using namespace os;

namespace os_priv
{
	enum Error_e
	{ E_OK, E_SKIP, E_CANCEL, E_ALL };
	struct CopyFileParams_s
	{
		CopyFileParams_s( const std::vector < std::string > cDstPaths, const std::vector < std::string > cSrcPaths, const Messenger & cViewTarget ):m_cDstPaths( cDstPaths ), m_cSrcPaths( cSrcPaths ), m_cViewTarget( cViewTarget )
		{
		}

		std::vector < std::string > m_cDstPaths;
		  std::vector < std::string > m_cSrcPaths;
		Messenger m_cViewTarget;
	};

	struct DeleteFileParams_s
	{
		DeleteFileParams_s( const std::vector < std::string > cPaths, const Messenger & cViewTarget ):m_cPaths( cPaths ), m_cViewTarget( cViewTarget )
		{
		}
		std::vector < std::string > m_cPaths;
		Messenger m_cViewTarget;
	};

	class DirKeeper:public Looper
	{
	      public:
		enum
		{ M_CHANGE_DIR = 1, M_COPY_FILES, M_MOVE_FILES, M_DELETE_FILES, M_RENAME_FILES, M_ENTRIES_ADDED, M_ENTRIES_REMOVED, M_ENTRIES_UPDATED };
		  DirKeeper( const Messenger & cTarget, const std::string & cPath );
		virtual void HandleMessage( Message * pcMessage );
		virtual bool Idle();
	      private:
		void SendAddMsg( const std::string & cName );
		void SendRemoveMsg( const std::string & cName, dev_t nDevice, ino_t nInode );
		void SendUpdateMsg( const std::string & cName, dev_t nDevice, ino_t nInode );

		enum state_t
		{ S_IDLE, S_READDIR };
		Messenger m_cTarget;
		Directory *m_pcCurrentDir;
		NodeMonitor m_cMonitor;
		state_t m_eState;
		bool m_bWaitForAddReply;
		bool m_bWaitForRemoveReply;
		bool m_bWaitForUpdateReply;

		struct MonitoringFileNode;
		struct FileNode
		{
			FileNode( const std::string & cName, dev_t nDevice, ino_t nInode ):m_cName( cName ), m_nDevice( nDevice ), m_nInode( nInode )
			{
			}

			bool operator<( const FileNode & cNode ) const
			{
				return ( m_nInode == cNode.m_nInode ? ( m_nDevice < cNode.m_nDevice ) : ( m_nInode < cNode.m_nInode ) );
			}
			bool operator<( const MonitoringFileNode & cNode ) const;

			mutable std::string m_cName;
			dev_t m_nDevice;
			ino_t m_nInode;
		};
		struct MonitoringFileNode:public FileNode
		{
			MonitoringFileNode( const std::string & cName, dev_t nDevice, ino_t nInode ):FileNode( cName, nDevice, nInode )
			{
				m_pcMonitor = NULL;
			}
			
			~MonitoringFileNode() {
				if( m_pcMonitor ) {
					delete m_pcMonitor;
				}
			}
			
			mutable NodeMonitor* m_pcMonitor;
		};


		std::set < MonitoringFileNode > m_cFileMap;
		std::set < FileNode > m_cAddMap;
		std::set < FileNode > m_cUpdateMap;
		std::set < FileNode > m_cRemoveMap;
	};

	bool DirKeeper::FileNode::operator<( const DirKeeper::MonitoringFileNode & cNode ) const
	{
		return ( m_nInode == cNode.m_nInode ? ( m_nDevice < cNode.m_nDevice ) : ( m_nInode < cNode.m_nInode ) );
	}

}

using namespace os_priv;

bool load_icon( const char *pzPath, void *pBuffer, bool bLarge )
{
	IconDir sDir;
	IconHeader sHeader;

	FILE *hFile = fopen( pzPath, "r" );

	if( hFile == NULL )
	{
		printf( "Failed to open file %s\n", pzPath );
		return ( false );
	}

	if( fread( &sDir, sizeof( sDir ), 1, hFile ) != 1 )
	{
		printf( "Failed to read icon dir\n" );
	}
	if( sDir.nIconMagic != ICON_MAGIC )
	{
		printf( "Files %s is not an icon\n", pzPath );
		return ( false );
	}
	for( int i = 0; i < sDir.nNumImages; ++i )
	{
		if( fread( &sHeader, sizeof( sHeader ), 1, hFile ) != 1 )
		{
			printf( "Failed to read icon header\n" );
		}
		if( sHeader.nWidth == 32 )
		{
			if( bLarge )
			{
				fread( pBuffer, 32 * 32 * 4, 1, hFile );
			}
			else
			{
				fseek( hFile, 32 * 32 * 4, SEEK_CUR );
			}
		}
		else if( sHeader.nWidth == 16 )
		{
			if( bLarge == false )
			{
				fread( pBuffer, 16 * 16 * 4, 1, hFile );
			}
			else
			{
				fseek( hFile, 16 * 16 * 4, SEEK_CUR );
			}
		}
	}
	fclose( hFile );
	return ( true );
}


DirKeeper::DirKeeper( const Messenger & cTarget, const std::string & cPath ):Looper( "dir_worker" ), m_cTarget( cTarget )
{
	m_pcCurrentDir = NULL;
	try
	{
		m_pcCurrentDir = new Directory( cPath );
		m_cMonitor.SetTo( m_pcCurrentDir, NWATCH_DIR | NWATCH_NAME, this, this );
	} catch( std::exception & )
	{
	}
	if( m_pcCurrentDir != NULL )
	{
		m_eState = S_READDIR;
	}
	else
	{
		m_eState = S_IDLE;
	}
	m_bWaitForAddReply = false;
	m_bWaitForRemoveReply = false;
	m_bWaitForUpdateReply = false;
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
				Directory *pcNewDir = new Directory( cNewPath );
				delete m_pcCurrentDir;

				m_pcCurrentDir = pcNewDir;
				m_eState = S_READDIR;
				m_cTarget.SendMessage( DirectoryView::M_CLEAR );
				m_cMonitor.SetTo( m_pcCurrentDir, NWATCH_DIR | NWATCH_NAME, this, this );
				m_cFileMap.clear();
				m_cAddMap.clear();
				m_cRemoveMap.clear();
				m_cUpdateMap.clear();
				m_bWaitForAddReply = false;
				m_bWaitForRemoveReply = false;
				m_bWaitForUpdateReply = false;
			}
			catch( std::exception & )
			{
			}
			break;
		}
	case M_ENTRIES_ADDED:
		{
			m_bWaitForAddReply = false;
			if( m_cAddMap.empty() == false )
			{
				Message cMsg( DirectoryView::M_ADD_ENTRY );
				int nCount = 0;

				while( m_cAddMap.empty() == false )
				{
					std::set < FileNode >::iterator i = m_cAddMap.begin();

					try
					{
						FSNode cNode( *m_pcCurrentDir, ( *i ).m_cName, O_RDONLY | O_NONBLOCK );
						struct stat sStat;

						cNode.GetStat( &sStat, false );
						uint32 anIcon[16 * 16];

						if( S_ISDIR( sStat.st_mode ) == false )
						{
							load_icon( "/system/icons/file.icon", anIcon, false );
						}
						else
						{
							load_icon( "/system/icons/folder.icon", anIcon, false );
						}
						cMsg.AddString( "name", ( *i ).m_cName );
						cMsg.AddData( "stat", T_ANY_TYPE, &sStat, sizeof( sStat ) );
						cMsg.AddData( "small_icon", T_ANY_TYPE, anIcon, 16 * 16 * 4 );

					}
					catch( std::exception & )
					{
					}
					m_cAddMap.erase( i );
					if( ++nCount > 5 )
					{
						break;
					}
				}
				m_bWaitForAddReply = true;
				m_cTarget.SendMessage( &cMsg );
			}
			break;
		}
	case M_ENTRIES_UPDATED:
		{
			m_bWaitForUpdateReply = false;
			if( m_cUpdateMap.empty() == false )
			{
				Message cMsg( DirectoryView::M_UPDATE_ENTRY );
				int nCount = 0;

				while( m_cUpdateMap.empty() == false )
				{
					std::set < FileNode >::iterator i = m_cUpdateMap.begin();

					try
					{
						FSNode cNode( *m_pcCurrentDir, ( *i ).m_cName, O_RDONLY | O_NONBLOCK );
						struct stat sStat;

						cNode.GetStat( &sStat, false );
						uint32 anIcon[16 * 16];

						if( S_ISDIR( sStat.st_mode ) == false )
						{
							load_icon( "/system/icons/file.icon", anIcon, false );
						}
						else
						{
							load_icon( "/system/icons/folder.icon", anIcon, false );
						}
						cMsg.AddString( "name", ( *i ).m_cName );
						cMsg.AddData( "stat", T_ANY_TYPE, &sStat, sizeof( sStat ) );
						cMsg.AddData( "small_icon", T_ANY_TYPE, anIcon, 16 * 16 * 4 );

					}
					catch( std::exception & )
					{
					}
					m_cUpdateMap.erase( i );
					if( ++nCount > 5 )
					{
						break;
					}
				}
				m_bWaitForUpdateReply = true;
				m_cTarget.SendMessage( &cMsg );
			}
			break;
		}
	case M_ENTRIES_REMOVED:
		{
			m_bWaitForRemoveReply = false;
			if( m_cRemoveMap.empty() == false )
			{
				Message cMsg( DirectoryView::M_REMOVE_ENTRY );
				int nCount = 0;

				while( m_cRemoveMap.empty() == false )
				{
					std::set < FileNode >::iterator i = m_cRemoveMap.begin();

					cMsg.AddInt32( "device", ( *i ).m_nDevice );
					cMsg.AddInt64( "node", ( *i ).m_nInode );
					m_cRemoveMap.erase( i );
					if( ++nCount > 10 )
					{
						break;
					}
				}
				m_cTarget.SendMessage( &cMsg );
				m_bWaitForRemoveReply = true;
			}
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
					std::string cName;

					pcMessage->FindString( "name", &cName );
					SendAddMsg( cName );
					break;
				}
			case NWEVENT_DELETED:
				{
					dev_t nDevice;
					int64 nInode;

					std::string cName;

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

					std::string cName;

					pcMessage->FindInt( "device", &nDevice );
					pcMessage->FindInt64( "node", &nInode );
					pcMessage->FindString( "name", &cName );
					SendUpdateMsg( cName, nDevice, nInode );
					break;
				}
			case NWEVENT_MOVED:
				{
					dev_t nDevice;
					int64 nOldDir;
					int64 nNewDir;
					int64 nInode;

					std::string cName;

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
							SendUpdateMsg( cName, nDevice, nInode );
						}
						else
						{
							if( nOldDir == int64 ( m_pcCurrentDir->GetInode() ) )
							{
								SendRemoveMsg( cName, nDevice, nInode );
							}
							else
							{
								SendAddMsg( cName );
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

void DirKeeper::SendUpdateMsg( const std::string & cName, dev_t nDevice, ino_t nInode )
{
	MonitoringFileNode cFileNode( cName, nDevice, nInode );
	std::set < MonitoringFileNode >::iterator i = m_cFileMap.find( cFileNode );

	if( i != m_cFileMap.end() )
	{
		try
		{
			if( cName.size() > 0 )
			{
				( *i ).m_cName = cName;
			}
			FSNode cNode( *m_pcCurrentDir, ( *i ).m_cName, O_RDONLY | O_NONBLOCK );
			struct stat sStat;

			cNode.GetStat( &sStat, false );

			FileNode cFileNode( ( *i ).m_cName, sStat.st_dev, sStat.st_ino );

//          std::set<FileNode>::iterator i = m_cRemoveMap.find( cFileNode );

			if( m_bWaitForUpdateReply )
			{
				m_cUpdateMap.insert( cFileNode );
			}
			else
			{
				uint32 anIcon[16 * 16];

				if( S_ISDIR( sStat.st_mode ) == false )
				{
					load_icon( "/system/icons/file.icon", anIcon, false );
				}
				else
				{
					load_icon( "/system/icons/folder.icon", anIcon, false );
				}

				Message cMsg( DirectoryView::M_UPDATE_ENTRY );

				cMsg.AddString( "name", ( *i ).m_cName );
				cMsg.AddData( "stat", T_ANY_TYPE, &sStat, sizeof( sStat ) );
				cMsg.AddData( "small_icon", T_ANY_TYPE, anIcon, 16 * 16 * 4 );

				m_cTarget.SendMessage( &cMsg );
				m_bWaitForUpdateReply = true;
			}
		}
		catch( std::exception & )
		{
		}
	}
	else
	{
		dbprintf( "Error: DirKeeper::SendUpdateMsg() got update message on non-existing node\n" );
	}

}

void DirKeeper::SendAddMsg( const std::string & cName )
{
	try
	{
		FSNode cNode( *m_pcCurrentDir, cName, O_RDONLY | O_NONBLOCK );
		struct stat sStat;

		cNode.GetStat( &sStat, false );

		FileNode cFileNode( cName, sStat.st_dev, sStat.st_ino );
		MonitoringFileNode cMFileNode( cName, sStat.st_dev, sStat.st_ino );

		std::set < FileNode >::iterator i = m_cRemoveMap.find( cFileNode );

		if( i != m_cRemoveMap.end() )
		{
			m_cRemoveMap.erase( i );
		}

		std::set < MonitoringFileNode >::iterator j = m_cFileMap.find( cMFileNode );

		if( j != m_cFileMap.end() )
		{
			SendUpdateMsg( cName, sStat.st_dev, sStat.st_ino );
			return;
		}
//      cMFileNode.m_cMonitor.SetTo( &cNode, NWATCH_STAT, this, this );
		( *m_cFileMap.insert( cMFileNode ).first ).m_pcMonitor = new NodeMonitor( &cNode, NWATCH_STAT, this, this );

		if( m_bWaitForAddReply )
		{
			m_cAddMap.insert( cFileNode );
		}
		else
		{
			uint32 anIcon[16 * 16];

			if( S_ISDIR( sStat.st_mode ) == false )
			{
				load_icon( "/system/icons/file.icon", anIcon, false );
			}
			else
			{
				load_icon( "/system/icons/folder.icon", anIcon, false );
			}

			Message cMsg( DirectoryView::M_ADD_ENTRY );

			cMsg.AddString( "name", cName );
			cMsg.AddData( "stat", T_ANY_TYPE, &sStat, sizeof( sStat ) );
			cMsg.AddData( "small_icon", T_ANY_TYPE, anIcon, 16 * 16 * 4 );

			m_cTarget.SendMessage( &cMsg );
			m_bWaitForAddReply = true;
		}
	}
	catch( ... /*std::exception& */  )
	{
	}
}

void DirKeeper::SendRemoveMsg( const std::string & cName, dev_t nDevice, ino_t nInode )
{
	FileNode cFileNode( cName, nDevice, nInode );
	MonitoringFileNode cMFileNode( cName, nDevice, nInode );
	std::set < FileNode >::iterator i = m_cAddMap.find( cFileNode );

	if( i != m_cAddMap.end() )
	{
		m_cAddMap.erase( i );
		return;
	}
	std::set < MonitoringFileNode >::iterator j = m_cFileMap.find( cMFileNode );

	if( j != m_cFileMap.end() )
	{
		m_cFileMap.erase( j );

		if( m_bWaitForRemoveReply )
		{
			m_cRemoveMap.insert( cFileNode );
		}
		else
		{
			Message cMsg( DirectoryView::M_REMOVE_ENTRY );

			cMsg.AddInt32( "device", nDevice );
			cMsg.AddInt64( "node", nInode );
			m_cTarget.SendMessage( &cMsg );
			m_bWaitForRemoveReply = true;
		}
	}
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
				return ( false );
			}
			String cName;
			if( m_pcCurrentDir->GetNextEntry( &cName ) == 1 )
			{
				if( ! (cName == ".") )
				{
					SendAddMsg( cName );
				}
				return ( true );
			}
			else
			{
				m_eState = S_IDLE;
				return ( false );
			}
			break;
		}
	case S_IDLE:
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

int ReadFileDlg( int nFile, void *pBuffer, int nSize, const char *pzPath, Error_e * peError )
{
      retry:
	int nLen = read( nFile, pBuffer, nSize );

	if( nLen >= 0 )
	{
		*peError = E_OK;
		return ( nLen );
	}
	else
	{
		if( errno == EINTR )
		{
			goto retry;
		}
	}
	std::strstream cMsg;

	cMsg << "Failed to read from source file: " << pzPath << "\nError: " << strerror( errno );

	Alert *pcAlert = new Alert( "Error:",
		std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
		"Skip", "Retry", "Cancel", NULL );

	switch ( pcAlert->Go() )
	{
	case 0:
		*peError = E_SKIP;
		break;
	case 1:
		goto retry;
	case 2:
		*peError = E_CANCEL;
		break;
	default:
		*peError = E_SKIP;
		break;
	}
	return ( nLen );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int WriteFileDlg( int nFile, const void *pBuffer, int nSize, const char *pzPath, Error_e * peError )
{
      retry:
	int nLen = write( nFile, pBuffer, nSize );

	if( nLen >= 0 )
	{
		*peError = E_OK;
		return ( nLen );
	}
	else
	{
		if( errno == EINTR )
		{
			goto retry;
		}
	}
	std::strstream cMsg;
	cMsg << "Failed to write to destination file: " << pzPath << "\nError: " << strerror( errno );

	Alert *pcAlert = new Alert( "Error:",
		std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
		"Skip", "Retry", "Cancel", NULL );

	switch ( pcAlert->Go() )
	{
	case 0:
		*peError = E_SKIP;
		break;
	case 1:
		goto retry;
	case 2:
		*peError = E_CANCEL;
		break;
	default:
		*peError = E_SKIP;
		break;
	}
	return ( nLen );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool CopyFile( const char *pzDst, const char *pzSrc, bool *pbReplaceFiles, bool *pbDontOverwrite, ProgressRequester * pcProgress )
{
	struct stat sSrcStat;
	struct stat sDstStat;
	bool bDirExists = false;

	if( pcProgress->DoCancel() )
	{
		return ( false );
	}

	while( lstat( pzSrc, &sSrcStat ) < 0 )
	{
		std::strstream cMsg;

		cMsg << "Failed to open source: " << pzSrc << "\nError: " << strerror( errno );
		Alert *pcAlert = new Alert( "Error:",
			std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
			"Skip", "Retry", "Cancel", NULL );

		switch ( pcAlert->Go() )
		{
		case 0:
			return ( true );
		case 1:
			break;
		case 2:
			return ( false );
		default:
			return ( true );
		}
	}

	bool bLoop = true;

	while( bLoop && lstat( pzDst, &sDstStat ) >= 0 )
	{
		if( sSrcStat.st_dev == sDstStat.st_dev && sSrcStat.st_ino == sDstStat.st_ino )
		{
			Alert *pcAlert = new Alert( "Error:", "Source and destination are the same, cant copy.", 0,
				"Ok", NULL );

			pcAlert->Go( NULL );
			return ( false );
		}

		if( S_ISDIR( sDstStat.st_mode ) )
		{
			if( S_ISDIR( sSrcStat.st_mode ) == false )
			{
				std::strstream cMsg;

				cMsg << "Can't replace directory  " << pzDst << " with a file\n";

				Alert *pcAlert = new Alert( "Error:",
					std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
					"Skip", "Retry", "Cancel", NULL );

				switch ( pcAlert->Go() )
				{
				case 0:
					return ( true );
				case 1:
					break;
				case 2:
					return ( false );
				default:
					return ( true );
				}
			}
			else
			{
				bDirExists = true;
				break;
			}
		}
		else
		{
			if( *pbDontOverwrite )
			{
				return ( true );
			}
			if( *pbReplaceFiles )
			{
				unlink( pzDst );
				break;
			}
			std::strstream cMsg;
			cMsg << "The destination file: " << pzDst << "already exists\nWould you like to replace it?";

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
				"Skip", "Skip all", "Replace", "Replace all", "Cancel", NULL );

			switch ( pcAlert->Go() )
			{
			case 0:	// Skip
				return ( true );
			case 1:	// Skip all
				*pbDontOverwrite = true;
				return ( true );
			case 2:	// Replace
				bLoop = false;
				unlink( pzDst );
				break;
			case 3:	// Replace all
				*pbReplaceFiles = true;
				bLoop = false;
				unlink( pzDst );
				break;
			case 4:	// Cancel
				return ( false );
			default:	// Bad
				return ( true );
			}
		}
	}

	if( S_ISDIR( sSrcStat.st_mode ) )
	{
		pcProgress->Lock();
		pcProgress->SetPathName( pzDst );
		pcProgress->Unlock();

		if( bDirExists == false )
		{
			while( mkdir( pzDst, sSrcStat.st_mode ) < 0 )
			{
				std::strstream cMsg;

				cMsg << "Failed to create directory: " << pzDst << "\nError: " << strerror( errno );
				Alert *pcAlert = new Alert( "Error:",
					std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
					"Skip", "Retry", "Cancel", NULL );

				switch ( pcAlert->Go() )
				{
				case 0:
					return ( true );
				case 1:
					break;
				case 2:
					return ( false );
				default:
					return ( true );
				}
			}
		}
		DIR *pDir;

		for( ;; )
		{
			pDir = opendir( pzSrc );
			if( pDir != NULL )
			{
				break;
			}
			std::strstream cMsg;
			cMsg << "Failed to open directory: " << pzSrc << "\nError: " << strerror( errno );
			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
				"Skip", "Retry", "Cancel", NULL );

			switch ( pcAlert->Go() )
			{
			case 0:
				return ( true );
			case 1:
				break;
			case 2:
				return ( false );
			default:
				return ( true );
			}
		}

		struct dirent *psEntry;

		while( ( psEntry = readdir( pDir ) ) != NULL )
		{
			if( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
			{
				continue;
			}
			Path cSrcPath( pzSrc );
			Path cDstPath( pzDst );

			cSrcPath.Append( psEntry->d_name );
			cDstPath.Append( psEntry->d_name );

			if( CopyFile( cDstPath.GetPath().c_str(), cSrcPath.GetPath(  ).c_str(), pbReplaceFiles, pbDontOverwrite, pcProgress ) == false )
			{
				closedir( pDir );
				return ( false );
			}
		}
		closedir( pDir );
	}
	else if( S_ISLNK( sSrcStat.st_mode ) )
	{
		pcProgress->Lock();
		pcProgress->SetFileName( Path( pzDst ).GetLeaf() );
		pcProgress->Unlock();
		printf( "Copy link %s to %s\n", pzSrc, pzDst );
	}
	else
	{
		pcProgress->Lock();
		pcProgress->SetFileName( Path( pzDst ).GetLeaf() );
		pcProgress->Unlock();

		int nSrcFile = -1;
		int nDstFile = -1;

		// Workaround for bug in egcs-1.1.2 (gives a warning about using uninitialized variables)
		nSrcFile = -1;
		nDstFile = -1;

		for( ;; )
		{
			nSrcFile = open( pzSrc, O_RDONLY );
			if( nSrcFile >= 0 )
			{
				break;
			}
			std::strstream cMsg;

			cMsg << "Failed to open source file: " << pzSrc << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
				"Skip", "Retry", "Cancel", NULL );

			switch ( pcAlert->Go() )
			{
			case 0:
				return ( true );
			case 1:
				break;
			case 2:
				return ( false );
			default:
				return ( true );
			}
		}
		for( ;; )
		{
			nDstFile = open( pzDst, O_WRONLY | O_CREAT | O_TRUNC, sSrcStat.st_mode );
			if( nDstFile >= 0 )
			{
				break;
			}
			std::strstream cMsg;

			cMsg << "Failed to create destination file: " << pzDst << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
				"Skip", "Retry", "Cancel", NULL );

			switch ( pcAlert->Go() )
			{
			case 0:
				close( nSrcFile );
				return ( true );
			case 1:
				break;
			case 2:
				close( nSrcFile );
				return ( false );
			default:
				close( nSrcFile );
				return ( true );
			}
		}
		int nTotLen = 0;

		for( ;; )
		{
			char anBuffer[4096];
			int nLen;
			Error_e eError;

			if( pcProgress->DoSkip() )
			{
				close( nSrcFile );
				close( nDstFile );
				unlink( pzDst );
				return ( true );
			}
			for( ;; )
			{
				nLen = ReadFileDlg( nSrcFile, anBuffer, 4096, pzSrc, &eError );
				if( nLen >= 0 )
				{
					break;
				}
				close( nSrcFile );
				close( nDstFile );
				unlink( pzDst );

				if( eError == E_SKIP )
				{
					return ( true );
				}
				else if( eError == E_CANCEL )
				{
					return ( false );
				}
				else
				{
					assert( !"Invalid return code from ReadFileDlg()" );
					return ( false );
				}
			}
			for( ;; )
			{
				int nError = WriteFileDlg( nDstFile, anBuffer, nLen, pzDst, &eError );

				if( nError >= 0 )
				{
					break;
				}

				close( nSrcFile );
				close( nDstFile );
				unlink( pzDst );

				if( eError == E_SKIP )
				{
					return ( true );
				}
				else if( eError == E_CANCEL )
				{
					return ( false );
				}
				else
				{
					assert( !"Invalid return code from WriteFileDlg()" );
					return ( false );
				}
			}
			nTotLen += nLen;
			if( nLen < 4096 )
			{
				break;
			}
		}
		close( nSrcFile );
		close( nDstFile );

		if( nTotLen != sSrcStat.st_size )
		{
			std::strstream cMsg;
			cMsg << "Mismatch between number of bytes read (" << nTotLen << ")\n" "and file size reported by file system (" << sSrcStat.st_size << ")!\n" "Seems like we might ended up corrupting the destination file\n";

			Alert *pcAlert = new Alert( "Warning:", cMsg.str(), 0, "Sorry", NULL );

			pcAlert->Go();
		}
//    printf( "Copy file %s to %s\n", pzSrc, pzDst );
	}
	return ( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool DeleteFile( const char *pzPath, ProgressRequester * pcProgress )
{
	struct stat sStat;

	if( pcProgress->DoCancel() )
	{
		return ( false );
	}

	while( lstat( pzPath, &sStat ) < 0 )
	{
		std::strstream cMsg;

		cMsg << "Failed to stat file: " << pzPath << "\nError: " << strerror( errno );
		Alert *pcAlert = new Alert( "Error:",
			std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
			"Skip", "Retry", "Cancel", NULL );

		switch ( pcAlert->Go() )
		{
		case 0:
			return ( true );
		case 1:
			break;
		case 2:
			return ( false );
		default:
			return ( true );
		}
	}
	if( S_ISDIR( sStat.st_mode ) )
	{
		pcProgress->Lock();
		pcProgress->SetPathName( pzPath );
		pcProgress->Unlock();

		DIR *pDir;

		for( ;; )
		{
			pDir = opendir( pzPath );
			if( pDir != NULL )
			{
				break;
			}

			std::strstream cMsg;

			cMsg << "Failed to open directory: " << pzPath << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
				"Skip", "Retry", "Cancel", NULL );

			switch ( pcAlert->Go() )
			{
			case 0:
				return ( true );
			case 1:
				break;
			case 2:
				return ( false );
			default:
				return ( true );
			}
		}
		struct dirent *psEntry;

		std::list <std::string > cFileList;

		while( ( psEntry = readdir( pDir ) ) != NULL )
		{
			if( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
			{
				continue;
			}
			Path cPath( pzPath );

			cPath.Append( psEntry->d_name );
			cFileList.push_back( cPath.GetPath() );
		}
		closedir( pDir );
		while( cFileList.empty() == false )
		{
			std::list <std::string >::iterator i = cFileList.begin();

			if( DeleteFile( ( *i ).c_str(), pcProgress ) == false )
			{
				return ( false );
			}
			cFileList.erase( i );
		}
//    printf( "delete dir %s\n", pzPath );
		while( rmdir( pzPath ) < 0 )
		{
			std::strstream cMsg;

			cMsg << "Failed to delete directory: " << pzPath << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
				"Skip", "Retry", "Cancel", NULL );

			switch ( pcAlert->Go() )
			{
			case 0:
				return ( true );
			case 1:
				break;
			case 2:
				return ( false );
			default:
				return ( true );
			}
		}
	}
	else
	{
		pcProgress->Lock();
		pcProgress->SetFileName( Path( pzPath ).GetLeaf() );
		pcProgress->Unlock();
//    printf( "delete file %s\n", pzPath );
		while( unlink( pzPath ) < 0 )
		{
			std::strstream cMsg;

			cMsg << "Failed to delete file: " << pzPath << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.pcount(  ) ), 0,
				"Skip", "Retry", "Cancel", NULL );

			switch ( pcAlert->Go() )
			{
			case 0:
				return ( true );
			case 1:
				break;
			case 2:
				return ( false );
			default:
				return ( true );
			}
		}
	}
	return ( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 DelFileThread( void *pData )
{
	DeleteFileParams_s *psArgs = ( DeleteFileParams_s * ) pData;

	std::strstream cMsg;
	const char *pzYes;
	const char *pzNo;

	if( psArgs->m_cPaths.size() == 1 )
	{
		struct stat sStat;

		if( lstat( psArgs->m_cPaths[0].c_str(), &sStat ) < 0 )
		{
			cMsg << "Failed to open file " << psArgs->m_cPaths[0].c_str() << "\nError: " << strerror( errno );
			Alert *pcAlert = new Alert( "Error:", cMsg.str(), 0, "Sorry", NULL );

			pcAlert->Go();
			delete psArgs;

			return ( 1 );
		}
		if( S_ISDIR( sStat.st_mode ) )
		{
			cMsg << "Are you sure you want to delete\n" "the directory \"" << Path( psArgs->m_cPaths[0].c_str() ).GetLeaf(  ).c_str() << "\"\n" "and all it's sub directories?";
		}
		else
		{
			cMsg << "Are you sure you want to delete the file\n" "\"" << Path( psArgs->m_cPaths[0].c_str() ).GetLeaf(  ).c_str() << "\"\n";
		}

		pzNo = "Leave it";
		pzYes = "Nuke it";
	}
	else
	{
		cMsg << "Are you sure you want to delete those " << psArgs->m_cPaths.size() << " entries?";
		pzNo = "Leave them";
		pzYes = "Nuke'em";
	}

	Alert *pcAlert = new Alert( "Verify file deletion:", cMsg.str(), 0, pzNo, pzYes, NULL );

	int nButton = pcAlert->Go();

	if( nButton != 1 )
	{
		delete psArgs;

		return ( 1 );
	}


	ProgressRequester *pcProgress = new ProgressRequester( Rect( 50, 20, 350, 150 ),
		"progress_window", "Delete file:", false );

	for( uint i = 0; i < psArgs->m_cPaths.size(); ++i )
	{
		if( DeleteFile( psArgs->m_cPaths[i].c_str(), pcProgress ) == false )
		{
			break;
		}
		Message cMsg( 125 );

		psArgs->m_cViewTarget.SendMessage( &cMsg );
	}

	pcProgress->Quit();

	delete psArgs;

	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void StartFileDelete( const std::vector < std::string > &cPaths, const Messenger & cViewTarget )
{
	DeleteFileParams_s *psParams = new DeleteFileParams_s( cPaths, cViewTarget );
	thread_id hTread = spawn_thread( "delete_file_thread", (void*)DelFileThread, 0, 0, psParams );

	if( hTread >= 0 )
	{
		resume_thread( hTread );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 CopyFileThread( void *pData )
{
	CopyFileParams_s *psParams = ( CopyFileParams_s * ) pData;

	ProgressRequester *pcProgress = new ProgressRequester( Rect( 50, 20, 350, 150 ),
		"progress_window", "Copy files:", true );

	for( uint i = 0; i < psParams->m_cSrcPaths.size(); ++i )
	{
		bool bReplaceFiles = false;
		bool bDontOverwrite = false;

		if( CopyFile( psParams->m_cDstPaths[i].c_str(), psParams->m_cSrcPaths[i].c_str(  ), &bReplaceFiles, &bDontOverwrite, pcProgress ) == false )
		{
			break;
		}

		Message cMsg( 125 );

		psParams->m_cViewTarget.SendMessage( &cMsg );
	}
	pcProgress->Quit();


	delete psParams;

	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void StartFileCopy( const std::vector < std::string > &cDstPaths, const std::vector < std::string > &cSrcPaths, const Messenger & cViewTarget )
{
	CopyFileParams_s *psParams = new CopyFileParams_s( cDstPaths, cSrcPaths, cViewTarget );
	thread_id hTread = spawn_thread( "copy_file_thread", (void*)CopyFileThread, 0, 0, psParams );

	if( hTread >= 0 )
	{
		resume_thread( hTread );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DirectoryView::State::State( ListView * pcView, const char *pzPath ):m_cPath( pzPath )
{
	for( uint i = 0; i < pcView->GetRowCount(); ++i )
	{
		if( pcView->IsSelected( i ) )
		{
			FileRow *pcRow = dynamic_cast < FileRow * >( pcView->GetRow( i ) );

			if( pcRow != NULL )
			{
				m_cSelection.push_back( pcRow->GetName() );
			}
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DirectoryView::DirectoryView( const Rect & cFrame, const String & cPath, uint32 nModeFlags, uint32 nResizeMask, uint32 nViewFlags ):ListView( cFrame, "_list_view", nModeFlags, nResizeMask, nViewFlags ), m_cPath( cPath.c_str() )
{
	m_pcCurReadDirSession = NULL;

	InsertColumn( "", 16 );	// Icon
	InsertColumn( "Name", 150 );
	InsertColumn( "Size", 50 );
	InsertColumn( "Attr", 70 );
	InsertColumn( "Date", 70 );
	InsertColumn( "Time", 70 );

	m_pcDirChangeMsg = NULL;

	m_nLastKeyDownTime = 0;
	m_pcIconBitmap = new Bitmap( 16, 16, CS_RGB32 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DirectoryView::~DirectoryView()
{
	delete m_pcIconBitmap;
}

void DirectoryView::AttachedToWindow()
{
	ListView::AttachedToWindow();
	m_pcDirKeeper = new DirKeeper( Messenger( this ), m_cPath.GetPath() );
	m_pcDirKeeper->Run();
//    ReRead();
}

void DirectoryView::DetachedFromWindow()
{
	m_pcDirKeeper->Quit();
	m_pcDirKeeper = NULL;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::KeyDown( const char *pzString, const char *pzRawString, uint32 nQualifiers )
{
	char nChar = pzString[0];

	if( isprint( nChar ) )
	{
		bigtime_t nTime = get_system_time();

		if( nTime < m_nLastKeyDownTime + 1000000 )
		{
			m_cSearchString.append( &nChar, 1 );
		}
		else
		{
			m_cSearchString = std::string( &nChar, 1 );
		}
		for( uint i = 0; i < GetRowCount(); ++i )
		{
			FileRow *pcRow = dynamic_cast < FileRow * >( GetRow( i ) );

			if( pcRow != NULL )
			{
				if( m_cSearchString.compare( 0, m_cSearchString.size(), pcRow->m_cName ) == 0 )
//				if( m_cSearchString.compare( pcRow->m_cName, 0, m_cSearchString.size() ) == 0 )
				{
					Select( i );
					MakeVisible( i, false );
					break;
				}
			}
		}
		m_nLastKeyDownTime = nTime;
	}
	else
	{
		switch ( pzString[0] )
		{
		case VK_DELETE:
			{
				std::vector < std::string > cPaths;
				for( int i = GetFirstSelected(); i <= GetLastSelected(  ); ++i )
				{
					if( IsSelected( i ) )
					{
						FileRow *pcRow = dynamic_cast < FileRow * >( GetRow( i ) );

						if( pcRow != NULL )
						{
							Path cPath = m_cPath;

							cPath.Append( pcRow->m_cName.c_str() );
							cPaths.push_back( cPath.GetPath() );
						}
					}
				}
				StartFileDelete( cPaths, Messenger( this ) );
				break;
			}
		case VK_BACKSPACE:
			m_cPath.Append( ".." );
			ReRead();
			PopState();
			DirChanged( m_cPath.GetPath() );
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
			ListView::KeyDown( pzString, pzRawString, nQualifiers );
			break;
		}
	}
}

int32 DirectoryView::ReadDirectory( void *pData )
{
	ReadDirParam *pcParam = ( ReadDirParam * ) pData;
	DirectoryView *pcView = pcParam->m_pcView;

	Window *pcWnd = pcView->GetWindow();

	if( pcWnd == NULL )
	{
		return ( 0 );
	}

	pcWnd->Lock();
	pcView->Clear();
	pcWnd->Unlock();

	DIR *pDir = opendir( pcView->GetPath().c_str(  ) );

	if( pDir == NULL )
	{
		dbprintf( "Error: DirectoryView::ReadDirectory() Failed to open %s\n", pcView->GetPath().c_str(  ) );
		goto error;
	}
	struct dirent *psEnt;

	while( pcParam->m_bCancel == false && ( psEnt = readdir( pDir ) ) != NULL )
	{
		struct stat sStat;

		if( strcmp( psEnt->d_name, "." ) == 0 /*|| strcmp( psEnt->d_name, ".." ) == 0 */  )
		{
			continue;
		}
		Path cFilePath( pcView->GetPath().c_str(  ) );

		cFilePath.Append( psEnt->d_name );

		stat( cFilePath.GetPath().c_str(), &sStat );

		FileRow *pcRow = new FileRow( pcView->m_pcIconBitmap, psEnt->d_name, sStat );

		if( S_ISDIR( pcRow->m_sStat.st_mode ) == false )
		{
			load_icon( "/system/icons/file.icon", pcRow->m_anIcon, false );
		}
		else
		{
			load_icon( "/system/icons/folder.icon", pcRow->m_anIcon, false );
		}
		pcWnd->Lock();
		pcView->InsertRow( pcRow );
		pcWnd->Unlock();
	}
	closedir( pDir );
      error:
	pcWnd->Lock();
	pcView->Sort();
	if( pcView->m_pcCurReadDirSession == pcParam )
	{
		pcView->m_pcCurReadDirSession = NULL;
	}
	pcWnd->Unlock();
	delete pcParam;

	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::ReRead()
{
	Message cMsg( DirKeeper::M_CHANGE_DIR );

	cMsg.AddString( "path", m_cPath.GetPath() );
	m_pcDirKeeper->PostMessage( &cMsg );

/*    
    if ( m_pcCurReadDirSession != NULL ) {
	m_pcCurReadDirSession->m_bCancel = true;
    }
    m_pcCurReadDirSession = new ReadDirParam( this );
  
    thread_id hTread = spawn_thread( "read_dir_thread", ReadDirectory, 0, 0, m_pcCurReadDirSession );
    if ( hTread >= 0 ) {
	resume_thread( hTread );
    }*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

String DirectoryView::GetPath() const
{
	return ( m_cPath.GetPath() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::SetPath( const String & cPath )
{
	while( m_cStack.size() > 0 )
	{
		m_cStack.pop();
	}
	m_cPath.SetTo( cPath.c_str() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::Invoked( int nFirstRow, int nLastRow )
{
	if( nFirstRow != nLastRow )
	{
		ListView::Invoked( nFirstRow, nLastRow );
		return;
	}
	FileRow *pcRow = dynamic_cast < FileRow * >( GetRow( nFirstRow ) );

	if( pcRow == NULL )
	{
		ListView::Invoked( nFirstRow, nLastRow );
		return;
	}
	if( S_ISDIR( pcRow->m_sStat.st_mode ) == false )
	{
		ListView::Invoked( nFirstRow, nLastRow );
		return;
	}
	bool bBack = false;

	if( strcmp( pcRow->m_cName.c_str(), ".." ) == 0 )
	{
		m_cPath.Append( ".." );
		bBack = true;
	}
	else
	{
		m_cStack.push( State( this, m_cPath.GetPath().c_str() ) );
		m_cPath.Append( pcRow->m_cName.c_str() );
	}
	ReRead();

	if( bBack )
	{
		PopState();
	}
	DirChanged( m_cPath.GetPath() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::PopState()
{
	if( m_cStack.empty() == false )
	{
		State & cState = m_cStack.top();

		for( uint i = 0; i < cState.m_cSelection.size(); ++i )
		{
			for( uint j = 0; j < GetRowCount(); ++j )
			{
				FileRow *pcRow = dynamic_cast < FileRow * >( GetRow( j ) );

				if( pcRow != NULL )
				{
					if( cState.m_cSelection[i] == pcRow->m_cName )
					{
						Select( j );
						MakeVisible( j );
						break;
					}
				}
			}
		}
		m_cStack.pop();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::SetDirChangeMsg( Message * pcMsg )
{
	delete m_pcDirChangeMsg;

	m_pcDirChangeMsg = pcMsg;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::DirChanged( const String & cNewPath )
{
	if( m_pcDirChangeMsg != NULL )
	{
		Invoke( m_pcDirChangeMsg );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool DirectoryView::DragSelection( const Point & cPos )
{
	int nFirstSel = GetFirstSelected();
	int nLastSel = GetLastSelected();
	int nNumRows = nLastSel - nFirstSel + 1;

	float vRowHeight = GetRow( 0 )->GetHeight( this ) + 3.0f;

	float vSelWidth = 0;

	Message cData( 1234 );

	for( int i = nFirstSel; i <= nLastSel; ++i )
	{
		FileRow *pcRow = dynamic_cast < FileRow * >( GetRow( i ) );

		if( pcRow != NULL && IsSelected( i ) )
		{
			float vLen = GetStringWidth( pcRow->m_cName.c_str() );

			if( vLen > vSelWidth )
			{
				vSelWidth = vLen;
			}
			Path cPath = m_cPath;

			cPath.Append( pcRow->m_cName.c_str() );
			cData.AddString( "file/path", cPath.GetPath() );
		}
	}

	vSelWidth += 18.0f;
	Rect cSelRect( 0, 0, vSelWidth - 1.0f, vRowHeight * nNumRows - 1 );
	Point cHotSpot( cPos.x, cPos.y - GetRowPos( nFirstSel ) );

	if( nLastSel - nFirstSel < 4 )
	{
		Bitmap cImage( (int)(cSelRect.Width() + 1.0f), (int)(cSelRect.Height(  ) + 1.0f), CS_RGB32, Bitmap::ACCEPT_VIEWS );
		View *pcView = new View( cSelRect, "" );

		cImage.AddChild( pcView );
		pcView->SetFgColor( 255, 255, 255, 255 );
		pcView->FillRect( cSelRect );

		Rect cBmRect( 0, 0, 15, 15 );
		Rect cTextRect( 18, 0, vSelWidth - 1.0f, vRowHeight - 1.0f );

		pcView->SetBgColor( 100, 100, 100 );
		for( int i = nFirstSel; i <= nLastSel; ++i )
		{
			FileRow *pcRow = dynamic_cast < FileRow * >( GetRow( i ) );

			if( pcRow != NULL && IsSelected( i ) )
			{
				pcView->SetDrawingMode( DM_OVER );
				pcRow->Paint( cBmRect, pcView, 0, false, false, false );
				pcView->SetDrawingMode( DM_COPY );
				pcRow->Paint( cTextRect, pcView, 6, false, false, false );
			}
			cBmRect += Point( 0.0f, vRowHeight );
			cTextRect += Point( 0.0f, vRowHeight );
		}
		cImage.Sync();
		BeginDrag( &cData, cHotSpot, &cImage );
	}
	else
	{
		BeginDrag( &cData, cHotSpot, cSelRect );
	}

	return ( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::HandleMessage( Message * pcMessage )
{
	switch ( pcMessage->GetCode() )
	{
	case M_CLEAR:
		Clear();
		break;
	case M_ADD_ENTRY:
		{
//          std::string cName;
			const char *pzName;
			const struct stat * psStat;
			size_t nSize;
			int nCount = 0;

			pcMessage->GetNameInfo( "stat", NULL, &nCount );
			for( int i = 0; i < nCount; ++i )
			{
				if( pcMessage->FindString( "name", &pzName, i ) != 0 )
				{
					break;
				}
				if( pcMessage->FindData( "stat", T_ANY_TYPE, ( const void ** )&psStat, &nSize, i ) != 0 )
				{
					break;
				}
				try
				{
					FileRow *pcRow = new FileRow( m_pcIconBitmap, pzName, *psStat );

					const void *pIconData;

					if( pcMessage->FindData( "small_icon", T_ANY_TYPE, &pIconData, &nSize, i ) == 0 )
					{
						memcpy( pcRow->m_anIcon, pIconData, 16 * 16 * 4 );
					}
					InsertRow( pcRow );
				}
				catch( std::exception & )
				{
				}
			}
			m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_ADDED, m_pcDirKeeper, m_pcDirKeeper );
			break;
		}
	case M_UPDATE_ENTRY:
		{
			const char *pzName;
			const struct stat * psStat;
			size_t nSize;
			int nCount = 0;

			pcMessage->GetNameInfo( "stat", NULL, &nCount );
			for( int i = 0; i < nCount; ++i )
			{
				if( pcMessage->FindString( "name", &pzName, i ) != 0 )
				{
					break;
				}
				if( pcMessage->FindData( "stat", T_ANY_TYPE, ( const void ** )&psStat, &nSize, i ) != 0 )
				{
					break;
				}
				try
				{
					for( ListView::const_iterator j = begin(); j != end(  ); ++j )
					{
						FileRow *pcRow = static_cast < FileRow * >( *j );

						if( pcRow->m_sStat.st_ino == psStat->st_ino && pcRow->m_sStat.st_dev == psStat->st_dev )
						{
							const void *pIconData;

							pcRow->m_cName = pzName;
							pcRow->m_sStat = *psStat;
							if( pcMessage->FindData( "small_icon", T_ANY_TYPE, &pIconData, &nSize, i ) == 0 )
							{
								memcpy( pcRow->m_anIcon, pIconData, 16 * 16 * 4 );
								InvalidateRow( j - begin(), ListView::INV_VISUAL );
							}
							break;
						}
//                      RemoveRow( j - begin() );
//                      InsertRow( pcRow );
					}
				}
				catch( std::exception & )
				{
				}
			}
			m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_UPDATED, m_pcDirKeeper, m_pcDirKeeper );
			break;
		}
	case M_REMOVE_ENTRY:
		{
			dev_t nDevice;
			int64 nInode;

			int nCount = 0;

			pcMessage->GetNameInfo( "device", NULL, &nCount );

			for( int i = 0; i < nCount; ++i )
			{
				pcMessage->FindInt( "device", &nDevice, i );
				pcMessage->FindInt64( "node", &nInode, i );

				for( ListView::const_iterator j = begin(); j != end(  ); ++j )
				{
					FileRow *pcRow = static_cast < FileRow * >( *j );

					if( int64 ( pcRow->m_sStat.st_ino ) == nInode && pcRow->m_sStat.st_dev == nDevice )
					{
						delete RemoveRow( j - begin() );

						break;
					}
				}
			}
			m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_REMOVED, m_pcDirKeeper, m_pcDirKeeper );
			break;
		}
	case 125:
		ReRead();
	default:
		ListView::HandleMessage( pcMessage );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FileRow::Paint( const Rect & cFrame, View * pcView, uint nColumn, bool bSelected, bool bHighlighted, bool bHasFocus )
{
	if( nColumn == 0 )
	{
		pcView->Sync();	// Make sure the previous icon is rendered before we overwrite the bitmap
		memcpy( m_pcIconBitmap->LockRaster(), m_anIcon, 16 * 16 * 4 );
//    ConvertIcon( m_pcIconBitmap, m_anIcon, false );
		pcView->DrawBitmap( m_pcIconBitmap, Rect( 0, 0, 15.0f, 15.0f ), cFrame );
		return;
	}

	font_height sHeight;

	pcView->GetFontHeight( &sHeight );

//  if ( bHighlighted ) {
//    pcView->SetFgColor( 0, 50, 200 );
//  } else if ( bSelected ) {
//    pcView->SetFgColor( 0, 0, 0 );
//  } else {
	pcView->SetFgColor( 255, 255, 255 );
//  }
	if( nColumn != 6 )
	{
		pcView->FillRect( cFrame );
	}

	float vFontHeight = sHeight.ascender + sHeight.descender;
	float vBaseLine = cFrame.top + ( cFrame.Height() + 1.0f ) / 2 - vFontHeight / 2 + sHeight.ascender;

	pcView->MovePenTo( cFrame.left + 3, vBaseLine );

	char zBuffer[256];
	const char *pzString = zBuffer;

	switch ( nColumn )
	{
	case 1:		// name
		pzString = m_cName.c_str();
		break;
	case 2:		// size
		if( S_ISDIR( m_sStat.st_mode ) )
		{
			strcpy( zBuffer, "<DIR>" );
		}
		else
		{
			sprintf( zBuffer, "%Ld", m_sStat.st_size );
		}
		break;
	case 3:		// attributes
		for( int i = 0; i < 10; ++i )
		{
			if( m_sStat.st_mode & ( 1 << i ) )
			{
				zBuffer[i] = "drwxrwxrwx"[9 - i];
			}
			else
			{
				zBuffer[i] = '-';
			}
		}
		zBuffer[10] = '\0';
		break;
	case 4:		// date
		{
			time_t nTime = m_sStat.st_ctime;

			strftime( zBuffer, 256, "%d/%b/%Y", localtime( &nTime ) );
			break;
		}
	case 5:		// time
		{
			time_t nTime = m_sStat.st_ctime;

			strftime( zBuffer, 256, "%H:%M:%S", localtime( &nTime ) );
			break;
		}
	case 6:		// name (for drag image)
		pzString = m_cName.c_str();
		break;
	default:
		printf( "Error: FileRow::Paint() - Invalid column %d\n", nColumn );
		return;
	}

/*
  if ( bHighlighted || (bSelected && bHasFocus) ) {
  pcView->SetFgColor( 255, 255, 255 );
  } else {
  pcView->SetFgColor( 0, 0, 0 );
  }
  */
	if( bHighlighted && nColumn == 1 )
	{
		pcView->SetFgColor( 255, 255, 255 );
		pcView->SetBgColor( 0, 50, 200 );
	}
	else if( bSelected && nColumn == 1 )
	{
		pcView->SetFgColor( 255, 255, 255 );
		pcView->SetBgColor( 0, 0, 0 );
	}
	else
	{
		pcView->SetBgColor( 255, 255, 255 );
		pcView->SetFgColor( 0, 0, 0 );
	}
	if( bSelected && nColumn == 1 )
	{
		Rect cRect = cFrame;

		cRect.right = cRect.left + pcView->GetStringWidth( pzString ) + 4;
		cRect.top = vBaseLine - sHeight.ascender - 1;
		cRect.bottom = vBaseLine + sHeight.descender + 1;
		pcView->FillRect( cRect, Color32_s( 0, 0, 0, 0 ) );
	}
	pcView->DrawString( pzString );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::MouseUp( const Point & cPosition, uint32 nButtons, Message * pcData )
{
	Highlight( 0, GetRowCount() - 1, false, false );
	if( pcData == NULL )
	{
		ListView::MouseUp( cPosition, nButtons, NULL );
		return;
	}
	StopScroll();

	const char *pzPath;

	if( pcData->FindString( "file/path", &pzPath ) != 0 )
	{
		return;
	}
	if( m_cPath == Path( pzPath ).GetDir() )
	{
		return;
	}

	FileRow *pcRow;

	int nSel = HitTest( cPosition );

	if( nSel != -1 )
	{
		pcRow = dynamic_cast < FileRow * >( GetRow( nSel ) );
	}
	else
	{
		pcRow = NULL;
	}

	Path cDstDir;

	if( pcRow != NULL && S_ISDIR( pcRow->m_sStat.st_mode ) )
	{
		cDstDir = m_cPath;
		cDstDir.Append( pcRow->GetName().c_str(  ) );
	}
	else
	{
		cDstDir = m_cPath;
	}

	std::vector < std::string > cSrcPaths;
	std::vector < std::string > cDstPaths;
	for( int i = 0; pcData->FindString( "file/path", &pzPath, i ) == 0; ++i )
	{
		Path cSrcPath( pzPath );
		Path cDstPath = cDstDir;

		cDstPath.Append( cSrcPath.GetLeaf() );
		cSrcPaths.push_back( pzPath );
		cDstPaths.push_back( cDstPath.GetPath() );
	}
	printf( "Start copy\n" );
	StartFileCopy( cDstPaths, cSrcPaths, Messenger( this ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::MouseMove( const Point & cNewPos, int nCode, uint32 nButtons, Message * pcData )
{
	if( pcData == NULL )
	{
		ListView::MouseMove( cNewPos, nCode, nButtons, NULL );
		return;
	}
	if( nCode == MOUSE_OUTSIDE )
	{
		return;
	}
	if( nCode == MOUSE_EXITED )
	{
		Highlight( 0, GetRowCount() - 1, false, false );
		StopScroll();
		return;
	}

	const char *pzPath;

	if( pcData->FindString( "file/path", &pzPath ) != 0 )
	{
		return;
	}

	Rect cBounds = GetBounds();

	if( cNewPos.y < cBounds.top + AUTOSCROLL_BORDER )
	{
		StartScroll( SCROLL_DOWN, false );
	}
	else if( cNewPos.y > cBounds.bottom - AUTOSCROLL_BORDER )
	{
		StartScroll( SCROLL_UP, false );
	}
	else
	{
		StopScroll();
	}

	int nSel = HitTest( cNewPos );

	if( nSel != -1 )
	{
		FileRow *pcRow = dynamic_cast < FileRow * >( GetRow( nSel ) );

		if( pcRow != NULL && S_ISDIR( pcRow->m_sStat.st_mode ) )
		{
			Path cRowPath = m_cPath;

			cRowPath.Append( pcRow->GetName().c_str(  ) );
			if( !( cRowPath == Path( pzPath ) ) )
			{
				Highlight( nSel, true, true );
				return;
			}
		}
	}
	Highlight( 0, GetRowCount() - 1, false, false );
}

void FileRow::AttachToView( View * pcView, int nColumn )
{
	if( nColumn == 0 )
	{
		m_avWidths[0] = 16.0f;
		return;
	}

	char zBuffer[256];
	const char *pzString = zBuffer;

	switch ( nColumn )
	{
	case 1:		// name
		pzString = m_cName.c_str();
		break;
	case 2:		// size
		if( S_ISDIR( m_sStat.st_mode ) )
		{
			strcpy( zBuffer, "<DIR>" );
		}
		else
		{
			sprintf( zBuffer, "%Ld", m_sStat.st_size );
		}
		break;
	case 3:		// attributes
		for( int i = 0; i < 10; ++i )
		{
			if( m_sStat.st_mode & ( 1 << i ) )
			{
				zBuffer[i] = "drwxrwxrwx"[9 - i];
			}
			else
			{
				zBuffer[i] = '-';
			}
		}
		zBuffer[10] = '\0';
		break;
	case 4:		// date
		{
			time_t nTime = m_sStat.st_ctime;

			strftime( zBuffer, 256, "%d/%b/%Y", localtime( &nTime ) );
			break;
		}
	case 5:		// time
		{
			time_t nTime = m_sStat.st_ctime;

			strftime( zBuffer, 256, "%H:%M:%S", localtime( &nTime ) );
			break;
		}
	case 6:		// name (for drag image)
		pzString = m_cName.c_str();
		break;
	default:
		printf( "Error: FileRow::AttachToView() - Invalid column %d\n", nColumn );
		return;
	}
	m_avWidths[nColumn] = pcView->GetStringWidth( pzString ) + 5.0f;
}

void FileRow::SetRect( const Rect & cRect, int nColumn )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float FileRow::GetWidth( View * pcView, int nColumn )
{
	return ( m_avWidths[nColumn] );

/*    if ( nColumn == 0 ) {
	return( 16.0f );
    }

    char  zBuffer[256];
    const char* pzString = zBuffer;
  
    switch( nColumn )
    {
	case 1:	// name
	    pzString = m_cName.c_str();
	    break;
	case 2:	// size
	    if ( S_ISDIR( m_sStat.st_mode ) ) {
		strcpy( zBuffer, "<DIR>" );
	    } else {
		sprintf( zBuffer, "%Ld", m_sStat.st_size );
	    }
	    break;
	case 3:	// attributes
	    for ( int i = 0 ; i < 10 ; ++i ) {
		if ( m_sStat.st_mode & (1<<i) ) {
		    zBuffer[i] = "drwxrwxrwx"[9-i];
		} else {
		    zBuffer[i] = '-';
		}
	    }
	    zBuffer[10] = '\0';
	    break;
	case 4:	// date
	{
	    time_t nTime = m_sStat.st_ctime;
	    strftime( zBuffer, 256, "%d/%b/%Y", localtime( &nTime ) );
	    break;
	}
	case 5:	// time
	{
	    time_t nTime = m_sStat.st_ctime;
	    strftime( zBuffer, 256, "%H:%M:%S", localtime( &nTime ) );
	    break;
	}
	case 6:	// name (for drag image)
	    pzString = m_cName.c_str();
	    break;
	default:
	    printf( "Error: FileRow::Paint() - Invalid column %d\n", nColumn );
	    return( 0.0f );
    }
    return( pcView->GetStringWidth( pzString ) + 5.0f );*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float FileRow::GetHeight( View * pcView )
{
	font_height sHeight;

	pcView->GetFontHeight( &sHeight );

	return ( std::max( 16.0f - 3.0f, sHeight.ascender + sHeight.descender ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool FileRow::HitTest( View * pcView, const Rect & cFrame, int nColumn, Point cPos )
{
	if( nColumn != 1 )
	{
		return ( true );
	}
	font_height sHeight;

	pcView->GetFontHeight( &sHeight );
	float nFontHeight = sHeight.ascender + sHeight.descender;
	float nBaseLine = cFrame.top + ( cFrame.Height() + 1.0f ) * 0.5f - nFontHeight / 2 + sHeight.ascender;
	Rect cRect( cFrame.left, nBaseLine - sHeight.ascender - 1.0f, cFrame.left + pcView->GetStringWidth( m_cName.c_str() ) + 4.0f, nBaseLine + sHeight.descender + 1.0f );

	return ( cRect.DoIntersect( cPos ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool FileRow::IsLessThan( const ListViewRow * pcOther, uint nColumn ) const
{
	const FileRow *pcRow = dynamic_cast < const FileRow * >( pcOther );

	if( NULL == pcRow )
	{
		return ( false );
	}

	if( S_ISDIR( m_sStat.st_mode ) != S_ISDIR( pcRow->m_sStat.st_mode ) )
	{
		return ( S_ISDIR( m_sStat.st_mode ) );
	}

	switch ( nColumn )
	{
	case 0:		// icon
	case 1:		// name
		return ( strcasecmp( m_cName.c_str(), pcRow->m_cName.c_str(  ) ) < 0 );
	case 2:		// size
		return ( m_sStat.st_size < pcRow->m_sStat.st_size );
	case 3:		// attributes
		return ( strcasecmp( m_cName.c_str(), pcRow->m_cName.c_str(  ) ) < 0 );
	case 4:		// date
	case 5:		// time
		return ( m_sStat.st_mtime < pcRow->m_sStat.st_mtime );
	default:
		printf( "Error: FileRow::IsLessThan() - Invalid column %d\n", nColumn );
		return ( false );
	}
}

