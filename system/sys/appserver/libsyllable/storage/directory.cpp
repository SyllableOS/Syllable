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

class Directory::Private {
	public:
	Private() {
		m_hDirIterator = NULL;
	}

	String	   m_cPathCache;
	DIR*	   m_hDirIterator;
};


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
	m = new Private;
}

/** Construct a directory from a path.
 * \par Description:
 * 	See: SetTo( const String& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Directory::Directory( const String & cPath, int nOpenMode ):FSNode( cPath, nOpenMode )
{
	m = new Private;

	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m->m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m->m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
}

/** Construct a directory from a path.
 * \par Description:
 * 	See: ( const Directory& cDir, const String& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Directory::Directory( const Directory & cDir, const String & cPath, int nOpenMode ):FSNode( cDir, cPath, nOpenMode )
{
	m = new Private;

	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m->m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m->m_hDirIterator == NULL )
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
	m = new Private;

	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m->m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m->m_hDirIterator == NULL )
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
	m = new Private;

	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m->m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m->m_hDirIterator == NULL )
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
	m = new Private;

	m->m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m->m_hDirIterator == NULL )
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
	m = new Private;

	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m->m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m->m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
}

Directory::~Directory()
{
	if( m->m_hDirIterator != NULL )
	{
		// the closedir() will close the FD so we must make sure ~FSNode() don't attempt to do the same.
		_SetFD( -1 );
		closedir( m->m_hDirIterator );
	}
	
	delete m;
}

status_t Directory::FDChanged( int nNewFD, const struct stat & sStat )
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
		if( m->m_hDirIterator != NULL )
		{
			// the closedir() will close the FD so we must make sure FSNode don't attempt to do the same.
			_SetFD( -1 );
			closedir( m->m_hDirIterator );
		}
		m->m_hDirIterator = hNewIter;
	}
	else if( m->m_hDirIterator != NULL )
	{
		// the closedir() will close the FD so we must make sure FSNode don't attempt to do the same.
		_SetFD( -1 );
		closedir( m->m_hDirIterator );
		m->m_hDirIterator = NULL;
	}
	return ( 0 );
}

/** Open a directory using a path.
 * \f status_t Directory::SetPath( const String& cPath, int nOpenMode = O_RDONLY )
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
 * \author	Dee Sharpe (demetrioussharpe@netscape.net)
 *****************************************************************************/
status_t Directory::SetPath( const String& cPath, int nOpenMode )
{
	status_t nError;
	
	if(nError = FSNode::SetTo(cPath, nOpenMode) > 0)
		return ( nError );
	
	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m->m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m->m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
	
	return ( nError );
}

/** Open a directory using a path.
 * \f status_t Directory::SetPath( const Path& cPath, int nOpenMode = O_RDONLY )
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
 * \author	Dee Sharpe (demetrioussharpe@netscape.net)
 *****************************************************************************/
status_t Directory::SetPath( const Path& cPath, int nOpenMode )
{
	String *cNewPath = new String(cPath.GetPath());
	status_t nError;
	
	if(nError = SetTo(cPath, nOpenMode) > 0)
	{
		delete cNewPath;
		return ( nError );
	}
	if( S_ISDIR( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node is not a directory", ENOTDIR );
	}
	m->m_hDirIterator = fopendir( GetFileDescriptor() );
	if( m->m_hDirIterator == NULL )
	{
		throw errno_exception( "Failed to create directory iterator" );
	}
	
	delete cNewPath;
	return ( nError );
}

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


status_t Directory::GetPath( String * pcPath ) const
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

/** Read the next directory entry.
 * \par Description:
 *	Reads the next entry (file or subdirectory) from the directory, if any, and stores
 *  the name in pcName.
 *
 *  This function can be used to iterate through the contents of a directory.
 *  The current position in the iteration is stored, and this function retrieves
 *  the name of the next entry in the iteration. Use Directory::Rewind() to reset thee
 *  iteration to the start of the directory.
 *
 *  The special entries "." and ".." are included in the iteration, if present in the directory.
 *
 *  Internally, this method uses the POSIX readdir() function, so all of the caveats for readdir
 *  apply to this method also. That includes unspecified behaviour if the directory contents are
 *  changed between one call to GetNextEntry() and the next.
 * 
 * \note Note the non-obvious return values: a value of 1 indicates that an entry was successfully read,
 *   0 indicates that the end of directory was reached.
 *
 * \param pcName
 *	String in which to store the name of the next directory entry.
 *
 * \retval Returns 1 if a directory entry was successfully read and its name stored in pcName,
 *   0 if the end of the directory has been reached, or a negative value in case of an error.
 *
 * \sa Directory::Rewind(), Directory::GetNextEntry( FileReference * ), readdir (POSIX function)
 *****************************************************************************/
status_t Directory::GetNextEntry( String * pcName )
{
	struct dirent *psEntry = readdir( m->m_hDirIterator );

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

/** \brief Read the next directory entry.
 * \par Description:
 *	Reads the next entry (file or subdirectory) from the directory, if any, and stores
 *  a reference to it in pcRef.
 *
 * \param pcRef Pointer to a FileReference in which to store the directory entry.
 *
 *  See Directory::GetNextEntry( String* ) for more information.
 *
 * \retval Returns 1 if an entry was successfully read and stored in pcRef, 0 if the end
 *  of the directory was reached, or a negative value in case of error.
 *
 * \sa Directory::GetNextEntry( String* ), Directory::Rewind(), readdir (POSIX function)
 */ 
status_t Directory::GetNextEntry( FileReference * pcRef )
{
	String cName;
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
	rewinddir( m->m_hDirIterator );
	return ( 0 );
}

/** Delete the directory
 * \par Warning: The Directory will be invalid if the call to Delete suceeds.
 *      You should destroy the Directory instance after calling Delete() to
 *      remove any references and ensure the directory is removed from the filesystem.
 * \author	Kristian Van Der Vliet (vanders@liqwyd.com)
 *****************************************************************************/

status_t Directory::Delete()
{
	String cPath;
	GetPath( &cPath );

	return rmdir( cPath.c_str() );
}

status_t Directory::CreateFile( const String & cName, File * pcFile, int nAccessMode )
{
	int nFile = based_open( GetFileDescriptor(), cName.c_str(  ), O_WRONLY | O_CREAT, nAccessMode );

	if( nFile < 0 )
	{
		return ( nFile );
	}
	close( nFile );
	return ( static_cast < FSNode * >( pcFile )->SetTo( *this, cName, O_RDWR ) );
}

status_t Directory::CreateDirectory( const String & cName, Directory * pcDir, int nAccessMode )
{
	int nError = based_mkdir( GetFileDescriptor(), cName.c_str(  ), nAccessMode );

	if( nError < 0 )
	{
		return ( nError );
	}
	return ( pcDir->SetTo( *this, cName, O_RDONLY ) );
}

status_t Directory::CreateSymlink( const String & cName, const String & cDestination, SymLink * pcLink )
{
	int nError = based_symlink( GetFileDescriptor(), cDestination.c_str(  ), cName.c_str(  ) );

	if( nError < 0 )
	{
		return ( nError );
	}
	return ( pcLink->SetTo( *this, cName, O_RDONLY ) );
}

