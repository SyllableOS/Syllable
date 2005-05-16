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

#include <gui/requesters.h>
#include <util/message.h>
#include <util/messenger.h>
#include <storage/path.h>
#include <storage/symlink.h>
#include <storage/operations.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#include <sstream>
#include <cassert>
#include <string>


using namespace os;


struct DeleteFileParams_s
{
	DeleteFileParams_s( const std::vector < os::String > &cPaths, const Messenger & cViewTarget, Message* pcMsg )
	:m_cPaths( cPaths ), m_cViewTarget( cViewTarget ), m_pcMsg( pcMsg )
	{
	}
	std::vector < os::String > m_cPaths;
	Messenger m_cViewTarget;
	Message* m_pcMsg;
};


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static bool OperationDeleteFile( const char *pzPath, ProgressRequester * pcProgress )
{
	struct stat sStat;

	if( pcProgress->DoCancel() )
	{
		return ( false );
	}

	while( lstat( pzPath, &sStat ) < 0 )
	{
		std::stringstream cMsg;

		cMsg << "Failed to stat file: " << pzPath << "\nError: " << strerror( errno );
		Alert *pcAlert = new Alert( "Error:",
			std::string( cMsg.str(), 0, cMsg.str().length() ), Alert::ALERT_WARNING, 0,
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

			std::stringstream cMsg;

			cMsg << "Failed to open directory: " << pzPath << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.str().length() ), Alert::ALERT_WARNING, 0,
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

			if( OperationDeleteFile( ( *i ).c_str(), pcProgress ) == false )
			{
				return ( false );
			}
			cFileList.erase( i );
		}
//    printf( "delete dir %s\n", pzPath );
		while( rmdir( pzPath ) < 0 )
		{
			std::stringstream cMsg;

			cMsg << "Failed to delete directory: " << pzPath << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.str().length() ), Alert::ALERT_WARNING, 0,
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
			std::stringstream cMsg;

			cMsg << "Failed to delete file: " << pzPath << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.str().length() ), 0,
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

static int32 DelFileThread( void *pData )
{
	DeleteFileParams_s *psArgs = ( DeleteFileParams_s * ) pData;

	std::stringstream cMsg;
	const char *pzYes;
	const char *pzNo;

	if( psArgs->m_cPaths.size() == 1 )
	{
		struct stat sStat;

		if( lstat( psArgs->m_cPaths[0].c_str(), &sStat ) < 0 )
		{
			cMsg << "Failed to open file " << psArgs->m_cPaths[0].c_str() << "\nError: " << strerror( errno );
			Alert *pcAlert = new Alert( "Error:", cMsg.str(), Alert::ALERT_WARNING, 0, "Ok", NULL );

			pcAlert->Go();
			
			psArgs->m_pcMsg->AddBool( "success", false );
			psArgs->m_cViewTarget.SendMessage( psArgs->m_pcMsg );

			delete( psArgs->m_pcMsg );
			delete( psArgs );
			
			
			return ( 1 );
		}
		if( S_ISDIR( sStat.st_mode ) )
		{
			cMsg << "Are you sure you want to delete\n" "the directory \"" << Path( psArgs->m_cPaths[0].c_str() ).GetLeaf(  ).c_str() << "\"\n" "and all it's sub directories?";
		}
		else if( S_ISLNK( sStat.st_mode ) )
		{
			cMsg << "Are you sure you want to delete the link\n" "\"" << Path( psArgs->m_cPaths[0].c_str() ).GetLeaf(  ).c_str() << "\"\n";
		}
		else
		{
			cMsg << "Are you sure you want to delete the file\n" "\"" << Path( psArgs->m_cPaths[0].c_str() ).GetLeaf(  ).c_str() << "\"\n";
		}

		pzNo = "No";
		pzYes = "Yes";
	}
	else
	{
		cMsg << "Are you sure you want to delete those " << psArgs->m_cPaths.size() << " entries?";
		pzNo = "No";
		pzYes = "Yes";
	}

	Alert *pcAlert = new Alert( "Verify file deletion:", cMsg.str(), Alert::ALERT_QUESTION, 0, pzNo, pzYes, NULL );

	int nButton = pcAlert->Go();

	if( nButton != 1 )
	{
		psArgs->m_pcMsg->AddBool( "success", false );
		psArgs->m_cViewTarget.SendMessage( psArgs->m_pcMsg );
		
		delete( psArgs->m_pcMsg );
		delete( psArgs );

		return ( 1 );
	}


	ProgressRequester *pcProgress = new ProgressRequester( Rect( 50, 20, 350, 150 ),
		"progress_window", "Delete file:", false );
	pcProgress->Lock();
	pcProgress->Show();
	pcProgress->Unlock();
	bool bSuccess = true;

	for( uint i = 0; i < psArgs->m_cPaths.size(); ++i )
	{
		if( OperationDeleteFile( psArgs->m_cPaths[i].c_str(), pcProgress ) == false )
		{
			bSuccess = false;
			break;
		}
		
	}

	pcProgress->PostMessage( os::M_QUIT );
	
	if( psArgs->m_pcMsg )
	{
		psArgs->m_pcMsg->AddBool( "success", bSuccess );
		psArgs->m_cViewTarget.SendMessage( psArgs->m_pcMsg );
	}

	delete( psArgs->m_pcMsg );
	delete( psArgs );

	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void os::delete_files( const std::vector < os::String > &cPaths, const Messenger & cMsgTarget, Message* pcFinishMsg )
{
	DeleteFileParams_s *psParams = new DeleteFileParams_s( cPaths, cMsgTarget, pcFinishMsg );
	thread_id hTread = spawn_thread( "delete_file_thread", (void*)DelFileThread, 0, 0, psParams );

	if( hTread >= 0 )
	{
		resume_thread( hTread );
	}
}
