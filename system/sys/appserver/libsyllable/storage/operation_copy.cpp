/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 The Syllable Team
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


enum Error_e
{ E_OK, E_SKIP, E_CANCEL, E_ALL };
struct CopyFileParams_s
{
	CopyFileParams_s( const std::vector < os::String > &cDstPaths, const std::vector < os::String > &cSrcPaths, const Messenger & cViewTarget, 
						Message* pcMsg, bool bReplace, bool bDontOverwrite )
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

/* From the coreutils patch */
#define CPATTR_BUF_SIZE 1024


static int cp_attribs (const char* src_path, const char* dst_path)
{
  int  nSrcFile = -1;
  DIR* pSrcDir = NULL;
  int  nDstFile = -1;
  bool status = 1;

  struct dirent* psEntry;

  nSrcFile = open (src_path, O_RDWR | O_NOTRAVERSE);
  nDstFile = open (dst_path, O_RDWR | O_NOTRAVERSE);
  if (nSrcFile < 0 || nDstFile < 0)
    goto END_CP; /* Failed to open source or dest file */

  pSrcDir = open_attrdir (nSrcFile);
  if (NULL == pSrcDir)
    goto END_CP; /* Failed to open source attrib dir */

  while ((psEntry = read_attrdir (pSrcDir)))
  {
    attr_info_s sInfo;

    if (stat_attr (nSrcFile, psEntry->d_name, &sInfo) == 0)
    {
      char zBuffer[CPATTR_BUF_SIZE];
      int index=0;
      int length;
      while ((length = (index+CPATTR_BUF_SIZE < sInfo.ai_size ?
        CPATTR_BUF_SIZE :
        sInfo.ai_size-index)))
      {
        if (read_attr (nSrcFile, psEntry->d_name, sInfo.ai_type, zBuffer,
            index, length ) != length)
          goto END_CP; /* Failed to read attribute */

        write_attr (nDstFile, psEntry->d_name, O_TRUNC, sInfo.ai_type,
          zBuffer, index, length);
        index+=length;
      }
    } 
    else 
    {
      /* Failed to stat attrib */
      goto END_CP;
    }
  }
  status = 0;

END_CP:
  close (nSrcFile);
  close (nDstFile);
  if (pSrcDir != NULL)
    close_attrdir (pSrcDir);
  return status;
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

static int ReadFileDlg( int nFile, void *pBuffer, int nSize, const char *pzPath, Error_e * peError )
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
	std::stringstream cMsg;

	cMsg << "Failed to read from source file: " << pzPath << "\nError: " << strerror( errno );

	Alert *pcAlert = new Alert( "Error:",
		std::string( cMsg.str(), 0, cMsg.str().length(  ) ), Alert::ALERT_WARNING, 0,
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

static int WriteFileDlg( int nFile, const void *pBuffer, int nSize, const char *pzPath, Error_e * peError )
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
	std::stringstream cMsg;
	cMsg << "Failed to write to destination file: " << pzPath << "\nError: " << strerror( errno );

	Alert *pcAlert = new Alert( "Error:",
		std::string( cMsg.str(), 0, cMsg.str().length(  ) ), Alert::ALERT_WARNING, 0,
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

bool OperationCopyFile( const char *pzDst, const char *pzSrc, bool *pbReplaceFiles, bool *pbDontOverwrite, bool bDeleteAfterCopy, ProgressRequester * pcProgress )
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
		std::stringstream cMsg;

		cMsg << "Failed to open source: " << pzSrc << "\nError: " << strerror( errno );
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

	bool bLoop = true;

	while( bLoop && lstat( pzDst, &sDstStat ) >= 0 )
	{
		if( sSrcStat.st_dev == sDstStat.st_dev && sSrcStat.st_ino == sDstStat.st_ino )
		{
			Alert *pcAlert = new Alert( "Error:", "Source and destination are the same. Copy not possible.", Alert::ALERT_WARNING, 0,
				"Ok", NULL );

			pcAlert->Go( NULL );
			return ( false );
		}

		if( S_ISDIR( sDstStat.st_mode ) )
		{
			if( S_ISDIR( sSrcStat.st_mode ) == false )
			{
				std::stringstream cMsg;

				cMsg << "Cannot replace directory  " << pzDst << " with a file\n";

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

	if( S_ISDIR( sSrcStat.st_mode ) )
	{
		pcProgress->Lock();
		pcProgress->SetPathName( pzDst );
		pcProgress->Unlock();

		if( bDirExists == false )
		{
			while( mkdir( pzDst, sSrcStat.st_mode ) < 0 )
			{
				std::stringstream cMsg;

				cMsg << "Failed to create directory: " << pzDst << "\nError: " << strerror( errno );
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
		
		cp_attribs( pzSrc, pzDst );
		
		DIR *pDir;

		for( ;; )
		{
			pDir = opendir( pzSrc );
			if( pDir != NULL )
			{
				break;
			}
			std::stringstream cMsg;
			cMsg << "Failed to open directory: " << pzSrc << "\nError: " << strerror( errno );
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

			if( OperationCopyFile( cDstPath.GetPath().c_str(), cSrcPath.GetPath(  ).c_str(), pbReplaceFiles, pbDontOverwrite, bDeleteAfterCopy, pcProgress ) == false )
			{
				closedir( pDir );
				return ( false );
			}
		}
		closedir( pDir );
		
		if( bDeleteAfterCopy )
			rmdir( pzSrc );
	}
	else if( S_ISLNK( sSrcStat.st_mode ) )
	{
		pcProgress->Lock();
		pcProgress->SetFileName( Path( pzDst ).GetLeaf() );
		pcProgress->Unlock();
		
//		printf( "Copy link %s to %s\n", os::SymLink( pzSrc ).ReadLink().c_str(), pzDst );
		symlink( os::SymLink( pzSrc ).ReadLink().c_str(), pzDst );
		
		cp_attribs( pzSrc, pzDst );
		
		if( bDeleteAfterCopy )
			unlink( pzSrc );
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
			std::stringstream cMsg;

			cMsg << "Failed to open source file: " << pzSrc << "\nError: " << strerror( errno );

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
		for( ;; )
		{
			nDstFile = open( pzDst, O_WRONLY | O_CREAT | O_TRUNC, sSrcStat.st_mode );
			if( nDstFile >= 0 )
			{
				break;
			}
			std::stringstream cMsg;

			cMsg << "Failed to create destination file: " << pzDst << "\nError: " << strerror( errno );

			Alert *pcAlert = new Alert( "Error:",
				std::string( cMsg.str(), 0, cMsg.str().length() ), Alert::ALERT_WARNING, 0,
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
		
		/* Copy attributes */
		cp_attribs( pzSrc, pzDst );

		if( nTotLen != sSrcStat.st_size )
		{
			std::stringstream cMsg;
			cMsg << "Mismatch between number of bytes read (" << nTotLen << ")\n" "and file size reported by file system (" << sSrcStat.st_size << ")!\n" "Seems like we might ended up corrupting the destination file\n";

			Alert *pcAlert = new Alert( "Warning:", cMsg.str(), Alert::ALERT_WARNING, 0, "Ok", NULL );
			
			pcAlert->Go();
		} 
		if( bDeleteAfterCopy )
			unlink( pzSrc );
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

static int32 CopyFileThread( void *pData )
{
	CopyFileParams_s *psParams = ( CopyFileParams_s * ) pData;

	ProgressRequester *pcProgress = new ProgressRequester( Rect( 50, 20, 350, 150 ),
		"progress_window", "Copy files:", true );
	pcProgress->Lock();
	pcProgress->Show();
	pcProgress->Unlock();
	bool bSuccess = true;
	
	bool bReplaceFiles = psParams->m_bReplace;
	bool bDontOverwrite = psParams->m_bDontOverwrite;


	for( uint i = 0; i < psParams->m_cSrcPaths.size(); ++i )
	{		
		if( OperationCopyFile( psParams->m_cDstPaths[i].c_str(), psParams->m_cSrcPaths[i].c_str(  ), &bReplaceFiles, &bDontOverwrite, false, pcProgress ) == false )
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

void os::copy_files( const std::vector < os::String > &cDstPaths, const std::vector < os::String > &cSrcPaths, const Messenger & cMsgTarget, Message* pcFinishMsg, 
					bool bReplace, bool bDontOverwrite )
{
	CopyFileParams_s *psParams = new CopyFileParams_s( cDstPaths, cSrcPaths, cMsgTarget, pcFinishMsg, bReplace, bDontOverwrite );
	thread_id hTread = spawn_thread( "copy_file_thread", (void*)CopyFileThread, 0, 0, psParams );

	if( hTread >= 0 )
	{
		resume_thread( hTread );
	}
}

