/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2004 The Syllable Team
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
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>

using namespace os;

enum Error_e
{ E_OK, E_SKIP, E_CANCEL, E_ALL };
struct MoveFileParams_s
{
	MoveFileParams_s( const std::vector < os::String > &cDstPaths, const std::vector < os::String > &cSrcPaths, const Messenger & cViewTarget, Message* pcMsg
					, bool bReplace, bool bDontOverwrite )
	:m_cDstPaths( cDstPaths ), m_cSrcPaths( cSrcPaths ), m_cViewTarget( cViewTarget ), m_pcMsg( pcMsg ), m_bReplace( bReplace ),
	m_bDontOverwrite( bDontOverwrite )
	{
	}
	std::vector < os::String > m_cDstPaths;
	std::vector < os::String > m_cSrcPaths;
	Messenger m_cViewTarget;
	Message* m_pcMsg;
	bool m_bReplace;
	bool m_bDontOverwrite;
};

/* operation_copy.cpp */
extern bool OperationCopyFile( const char *pzDst, const char *pzSrc, bool *pbReplaceFiles, bool *pbDontOverwrite, bool bDeleteAfterCopy, ProgressRequester * pcProgress );

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static bool OperationMoveFile( const char *pzDst, const char *pzSrc, bool *pbReplaceFiles, bool *pbDontOverwrite, ProgressRequester * pcProgress )
{
	//std::cout<<"Move "<<pzSrc<<" to "<<pzDst<<std::endl;
	struct stat sSrcStat;
	struct stat sDstStat;
	struct stat sDstDirStat;
	
	os::Path cDstDir = os::Path( pzDst ).GetDir();
	
	/* Check if the source exist */
	while( lstat( pzSrc, &sSrcStat ) < 0 )
	{
		std::stringstream cMsg;

		cMsg << pzSrc << " does not exist"<<std::endl;

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
	
	/* Check if the destination directory already exist */
	while( lstat( cDstDir.GetPath().c_str(), &sDstDirStat ) < 0 )
	{
		std::stringstream cMsg;

		cMsg << cDstDir.GetPath().c_str() << " does not exist"<<std::endl;

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
	
	/* Check if the source and destination are on the same device */
	if( sSrcStat.st_dev != sDstDirStat.st_dev )
	{
	
		bool bResult = OperationCopyFile( pzDst, pzSrc, pbReplaceFiles, pbDontOverwrite, true, pcProgress );
		
		return( bResult );
	}
	
	bool bLoop = true;
	
	/* Check if the destination already exist */
	while( bLoop && lstat( pzDst, &sDstStat ) >= 0 )
	{
		if( sSrcStat.st_dev == sDstStat.st_dev && sSrcStat.st_ino == sDstStat.st_ino )
		{
			Alert *pcAlert = new Alert( "Error:", "Source and destination are the same. Move not possible.", Alert::ALERT_WARNING, 0,
				"Ok", NULL );

			pcAlert->Go( NULL );
			return ( false );
		}
		
		/* Only proceed if both are files */
		if( S_ISDIR( sSrcStat.st_mode ) || S_ISDIR( sDstStat.st_mode ) )
		{
			std::stringstream cMsg;

			cMsg << "Cannot replace directory: " << pzDst << " with: "<<pzSrc<<"\n";

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
		} else if( *pbDontOverwrite ) {
			return( true );
		} else if( *pbReplaceFiles ) {
			bLoop = false;
			unlink( pzDst );
		} else {
			
			std::stringstream cMsg;
			cMsg << "The destination file: " << pzDst << " already exists\nWould you like to replace it?";

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.str().length(  ) ), Alert::ALERT_QUESTION, 0,
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
	
	for( ;; )
	{
		if( rename( pzSrc, pzDst ) >= 0 )
		{
			return( true );
		}
		
		std::stringstream cMsg;

		cMsg << "Failed to move: " << pzSrc << " to: " << pzDst<<std::endl;

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

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 MoveFileThread( void *pData )
{
	MoveFileParams_s *psParams = ( MoveFileParams_s * ) pData;
	
	ProgressRequester *pcProgress = new ProgressRequester( Rect( 50, 20, 350, 150 ),
		"progress_window", "Move files:", true );
	pcProgress->Lock();
	pcProgress->Show();
	pcProgress->Unlock();
	bool bSuccess = true;

	bool bReplaceFiles = psParams->m_bReplace;
	bool bDontOverwrite = psParams->m_bDontOverwrite;


	for( uint i = 0; i < psParams->m_cSrcPaths.size(); ++i )
	{		
		if( OperationMoveFile( psParams->m_cDstPaths[i].c_str(), psParams->m_cSrcPaths[i].c_str() , &bReplaceFiles, &bDontOverwrite, pcProgress ) == false )
		{
			bSuccess = false;
			break;
		}
	}
	
	pcProgress->PostMessage( os::M_QUIT );
	
	if( psParams->m_pcMsg )
	{
		psParams->m_pcMsg->AddBool( "success", bSuccess );
		psParams->m_cViewTarget.SendMessage( psParams->m_pcMsg );
	}

	delete( psParams->m_pcMsg );
	delete( psParams );

	return ( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void os::move_files( const std::vector < os::String > &cDstPaths, const std::vector < os::String > &cSrcPaths, const Messenger & cMsgTarget, Message* pcFinishMsg, 
					bool bReplace, bool bDontOverwrite )
{
	MoveFileParams_s *psParams = new MoveFileParams_s( cDstPaths, cSrcPaths, cMsgTarget, pcFinishMsg, bReplace, bDontOverwrite );
	thread_id hTread = spawn_thread( "move_file_thread", (void*)MoveFileThread, 0, 0, psParams );

	if( hTread >= 0 )
	{
		resume_thread( hTread );
	}
}
