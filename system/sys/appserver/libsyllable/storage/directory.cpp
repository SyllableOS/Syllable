
/*  libatheos.so - the highlevel API library for AtheOS
 *  Copyright (C) 2001  Kurt Skauen
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
#include <unistd.h>
#include <limits.h>
#include <storage/directory.h>
#include <storage/filereference.h>
#include <storage/file.h>
#include <storage/symlink.h>
#include <storage/path.h>

#include <util/exceptions.h>

using namespace os;

/** Default contructor.
 * \par Description:
 *	Initiate the instance to a know but invalid state.
 *	The instance must be successfully initialized with one
 *	of the SetTo() members before it can be used.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Directory::Directory()
{
	m_hDirIterator = NULL;
}

/** Construct a directory from a path.
 * \par Description:
 * 	See: SetTo( const std::string& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Directory::Directory( const std::string & cPath, int nOpenMode ):FSNode( cPath, nOpenMode )
{
	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
}

/** Construct a directory from a path.
 * \par Description:
 * 	See: ( const Directory& cDir, const std::string& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Directory::Directory( const Directory & cDir, const std::string & cPath, int nOpenMode ):FSNode( cDir, cPath, nOpenMode )
{
	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
}

/** Construct a directory from a path
 * \par Description:
 *	See: SetTo( const FileReference& cRef, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Directory::Directory( const FileReference & cRef, int nOpenMode ):FSNode( cRef, nOpenMode )
{
	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
}

/** Construct a directory from a path
 * \par Description:
 *	See: SetTo( const FSNode& cNode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Directory::Directory( const FSNode & cNode ):FSNode( cNode )
{
	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
}

/** Copy constructor
 * \par Description:
 *	Make a independent copy of another directory.
 *
 *	The new directory will consume a new file descriptor so the
 *	copy might fail (throwing an errno_exception exception) if the
 *	process run out of file descriptors.
 *
 * \param cDir
 *	The directory to copy
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


Directory::Directory( const Directory & cDir ):FSNode( cDir )
{
	m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
}

/** Construct a directory object from a open filedescriptor.
 * \par Description:
 *	Construct a directory object from a open filedescriptor.
 *	The file descriptor must be referencing a directory or
 *	an errno_exception with the ENOTDIR error code will be thrown.
 * \param nFD
 *	An open filedescriptor referencing a directory.
 * \since 0.3.7
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


Directory::Directory( int nFD ):FSNode( nFD )
{
	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
}

Directory::~Directory()
{
	if( m_hDirIterator != NULL )
	{
		// the closedir() will close the FD so we must make sure ~FSNode() don't attempt to do the same.
		_SetFD( -1 );
		closedir( m_hDirIterator );
	}
}

status_t Directory::FDChanged( int nNewFD, const struct::stat & sStat )
{
	if( nNewFD >= 0 && S_ISDIR( sStat.st_mode ) == false )
	{
		errno = ENOTDIR;
		return ( -1 );
	}
	if( nNewFD != -1 )
	{
		DIR *hNewIter = fopendir( nNewFD );

		if( hNewIter == NULL )
		{
			return ( -1 );
		}
		if( m_hDirIterator != NULL )
		{
			// the closedir() will close the FD so we must make sure FSNode don't attempt to do the same.
			_SetFD( -1 );
			closedir( m_hDirIterator );
		}
		m_hDirIterator = hNewIter;
	}
	else if( m_hDirIterator != NULL )
	{
		// the closedir() will close the FD so we must make sure FSNode don't attempt to do the same.
		_SetFD( -1 );
		closedir( m_hDirIterator );
		m_hDirIterator = NULL;
	}
	return ( 0 );
}

/*
void Directory::Unset()
{
    if ( m_hDirIterator != NULL ) {
	  // the closedir() will close the FD so we must make sure FSNode::Unset() don't attempt to do the same.
	_SetFD( -1 ); 
	closedir( m_hDirIterator );
    }
    FSNode::Unset();
}
*/

/** Open a directory using a path.
 * \f status_t Directory::SetTo( const std::string& cPath, int nOpenMode = O_RDONLY )
 * \par Description:
 *	Open the directory pointet to by \p cPath. The path must
 *	be valid and it must point to a directory.
 *
 * \param cPath
 *	The directory to open.
 * \param nOpenMode
 *	Flags describing how to open the directory. Only O_RDONLY,
 *	O_WRONLY, and O_RDWR are relevant to directories. Take a look
 *	at the os::FSNode documentation for a more detailed
 *	description of open modes.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


/** Get the absolute path of the directory
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


status_t Directory::GetPath( std::string * pcPath ) const
{
	char zBuffer[PATH_MAX];
	status_t nError;

	nError = get_directory_path( GetFileDescriptor(), zBuffer, PATH_MAX );
	if( nError == PATH_MAX )
	{
		errno = ENAMETOOLONG;
		return ( -1 );
	}
	if( nError < 0 )
	{
		return ( nError );
	}
	*pcPath = zBuffer;
	return ( 0 );
}

status_t Directory::GetNextEntry( std::string * pcName )
{
	struct dirent *psEntry = readdir( m_hDirIterator );

	if( psEntry == NULL )
	{
		return ( 0 );
	}
	else
	{
		*pcName = psEntry->d_name;
		return ( 1 );
	}
}

status_t Directory::GetNextEntry( FileReference * pcRef )
{
	std::string cName;
	status_t nError = GetNextEntry( &cName );

	if( nError != 1 )
	{
		return ( nError );
	}
	nError = pcRef->SetTo( *this, cName, false );
	if( nError < 0 )
	{
		return ( nError );
	}
	return ( 1 );
}

status_t Directory::Rewind()
{
	rewinddir( m_hDirIterator );
	return ( 0 );
}



status_t Directory::CreateFile( const std::string & cName, File * pcFile, int nAccessMode )
{
	int nFile = based_open( GetFileDescriptor(), cName.c_str(  ), O_WRONLY | O_CREAT, nAccessMode );

	if( nFile < 0 )
	{
		return ( nFile );
	}
	close( nFile );
	return ( static_cast < FSNode * >( pcFile )->SetTo( *this, cName, O_RDWR ) );
}

status_t Directory::CreateDirectory( const std::string & cName, Directory * pcDir, int nAccessMode )
{
	int nError = based_mkdir( GetFileDescriptor(), cName.c_str(  ), nAccessMode );

	if( nError < 0 )
	{
		return ( nError );
	}
	return ( pcDir->SetTo( *this, cName, O_RDONLY ) );
}

status_t Directory::CreateSymlink( const std::string & cName, const std::string & cDestination, SymLink * pcLink )
{
	int nError = based_symlink( GetFileDescriptor(), cDestination.c_str(  ), cName.c_str(  ) );

	if( nError < 0 )
	{
		return ( nError );
	}
	return ( pcLink->SetTo( *this, cName, O_RDONLY ) );
}
