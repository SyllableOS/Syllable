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

#include <unistd.h>
#include <assert.h>
#include <storage/fsnode.h>
#include <storage/filereference.h>
#include <storage/directory.h>
#include <storage/path.h>

#include <util/exceptions.h>

using namespace os;

class FSNode::Private
{
	public:
	Private() {
		m_nFD = -1;
		m_hAttrDir = NULL;
	}

    int  m_nFD;
    DIR* m_hAttrDir;
};

/** Default contructor.
 * \par Description:
 *	Initiate the FSNode to a known but "invalid" state.
 *	The node must be initialize with one of the SetTo()
 *	members before any other members can be called.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

FSNode::FSNode()
{
	m = new Private;
}

/** Construct a FSNode from a file path.
 * \par Description:
 *	See: SetTo( const String& cPath, int nOpenMode )
 * \par Note:
 *	Since constructors can't return error codes it will throw an
 *	os::errno_exception in the case of failure. The error code can
 *	be retrieved from the exception object.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

FSNode::FSNode( const String & cPath, int nOpenMode )
{
	m = new Private;

	if( cPath.size() > 1 && cPath[0] == '~' && cPath[1] == '/' )
	{
		const char *pzHome = getenv( "HOME" );

		if( pzHome == NULL )
		{
			throw( errno_exception( "HOME environment variable not set", ENOENT ) );
		}
		String cRealPath = pzHome;
		cRealPath.str().insert( cRealPath.end(), cPath.begin(  ) + 1, cPath.end(  ) );
		m->m_nFD = open( cRealPath.c_str(), nOpenMode, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR );
	}
	else if( cPath.size() > 1 && cPath[0] == '^' && cPath[1] == '/' )
	{
		int nDir = open_image_file( IMGFILE_BIN_DIR );

		if( nDir < 0 )
		{
			throw errno_exception( "Failed to executable directory node" );
		}
		m->m_nFD = based_open( nDir, cPath.c_str() + 2, nOpenMode );
		int nOldErr = errno;

		close( nDir );
		if( m->m_nFD < 0 )
		{
			errno = nOldErr;
		}
	}
	else
	{
		m->m_nFD = open( cPath.c_str(), nOpenMode, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR );
	}
	m->m_hAttrDir = NULL;
	if( m->m_nFD < 0 )
	{
		throw errno_exception( "Failed to open node" );
	}
	if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
	{
		close( m->m_nFD );
		throw errno_exception( "Failed to stat node" );
	}
}

/** Construct a FSNode from directory and a name inside that directory.
 * \par Description:
 *	See: SetTo( const Directory& cDir, const String& cPath, int nOpenMode )
 * \par Note:
 *	Since constructors can't return error codes it will throw an
 *	os::errno_exception in the case of failure. The error code can
 *	be retrieved from the exception object.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

FSNode::FSNode( const Directory & cDir, const String & cPath, int nOpenMode )
{
	m = new Private;

	if( cDir.IsValid() == false )
	{
		throw errno_exception( "Invalid directory", EINVAL );
	}
	m->m_nFD = based_open( cDir.GetFileDescriptor(), cPath.c_str(  ), nOpenMode, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR );
	m->m_hAttrDir = NULL;
	if( m->m_nFD < 0 )
	{
		throw errno_exception( "Failed to open node" );
	}
	if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
	{
		close( m->m_nFD );
		throw errno_exception( "Failed to stat node" );
	}
}

/** Construct a FSNode from a file reference.
 * \par Description:
 *	See: SetTo( const FileReference& cRef, int nOpenMode )
 * \par Note:
 *	Since constructors can't return error codes it will throw an
 *	os::errno_exception in the case of failure. The error code can
 *	be retrieved from the exception object.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

FSNode::FSNode( const FileReference & cRef, int nOpenMode )
{
	m = new Private;

	if( cRef.IsValid() == false )
	{
		throw errno_exception( "Invalid directory", EINVAL );
	}
	m->m_nFD = based_open( cRef.GetDirectory().GetFileDescriptor(  ), cRef.GetName(  ).c_str(  ), nOpenMode );
	m->m_hAttrDir = NULL;
	if( m->m_nFD < 0 )
	{
		throw errno_exception( "Failed to open node" );
	}
	if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
	{
		close( m->m_nFD );
		throw errno_exception( "Failed to stat node" );
	}
}

/** Construct a FSNode from a file descriptor.
 * \par Description:
 *	See: SetTo( int nFD )
 * \par Note:
 *	Since constructors can't return error codes it will throw an
 *	os::errno_exception in the case of failure. The error code can
 *	be retrieved from the exception object.
 * \since 0.3.7
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

FSNode::FSNode( int nFD )
{
	m = new Private;

	m->m_nFD = nFD;
	m->m_hAttrDir = NULL;
	if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
	{
		close( m->m_nFD );
		throw errno_exception( "Failed to stat node" );
	}
}

/** Copy contructor
 * \par Description:
 *	Copy an existing node. If the node can't be copyed an
 *	os::errno_exception will be thrown. Each node consume
 *	a file-descriptor so running out of FD's will cause
 *	the copy to fail.
 * \par Note:
 *	The attribute directory iterator will not be cloned so when
 *	FSNode::GetNextAttrName() is called for the first time it will
 *	return the first attribute name even if the iterator was not
 *	located at the beginning of the originals attribute directory.
 * \param cNode
 *	The node to copy.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

FSNode::FSNode( const FSNode & cNode )
{
	m = new Private;

	m->m_nFD = fcntl( cNode.m->m_nFD, F_COPYFD, -1 );
	m->m_hAttrDir = NULL;
	if( m->m_nFD < 0 )
	{
		throw errno_exception( "Failed to duplicate FD" );
	}
	m_sStatCache = cNode.m_sStatCache;
}

/** Destructor
 * \par Description:
 *	Will close the file descriptor and release other resources may
 *	consumed by the FSNode.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

FSNode::~FSNode()
{
	if( m->m_hAttrDir != NULL )
	{
		close_attrdir( m->m_hAttrDir );
	}
	if( m->m_nFD != -1 )
	{
		close( m->m_nFD );
	}

	delete m;
}

void FSNode::_SetFD( int nFD )
{
	m->m_nFD = nFD;
}

/** Check if the node has been properly initialized.
 * \par Description:
 *	Return true if the the object actually reference
 *	a real FS node. All other access functions will fail
 *	if the object is not fully initializesed eighter through
 *	one of the non-default constructors or with one of the
 *	SetTo() members.
 *
 * \return
 *	True if the object is fully initialized false otherwhice.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool FSNode::IsValid() const
{
	return ( m->m_nFD >= 0 );
}

/** Open a node using a path.
 * \par Description:
 *	Open a node by path. The path must be valid and the
 *	process must have access to it but it can point to
 *	any kind of FS-node (file, directory, symlink).
 *
 *	The path can start with "~/" to make it relative to the
 *	current users home directory or it can start with "^/"
 *	to make it relative to the directory where the executable
 *	our application was started from lives in. This path
 *	expansion is performed by the os::FSNode class itself
 *	and is not part of the low-level filesystem.
 *
 *	The \p nOpenMode should be a compination of any of the O_*
 *	flags defined in <fcntl.h>. Their meaning is the same as when
 *	opening a file with the open() POSIX function except you can
 *	not create a file by setting the O_CREAT flag.
 *
 *	The following flags are accepted:
 *
 *	- O_RDONLY	open the node read-only
 *	- O_WRONLY	open the node write-only
 *	- O_RDWR	open the node for both reading and writing
 *	- O_TRUNC	trunate the size to 0 (only valid for files)
 *	- O_APPEND	automatically move the file-pointer to the end
 *			of the file before each write (only valid for files)
 *	- O_NONBLOCK	open the file in non-blocking mode
 *	- O_DIRECTORY	fail if \p cPath don't point at a directory
 *	- O_NOTRAVERSE	open the symlink it self rather than it's target
 *			if \p cPath points at a symlink
 *
 * \par Note:
 *	If this call fail the old state of the FSNode will remain
 *	unchanged
 * \param cPath
 *	Path pointing at the node to open.
 * \param nOpenMode
 *	Flags controlling how to open the node.
 * \return
 *	On success 0 is returned. On error -1 is returned and a
 *	error code is assigned to the global variable "errno".
 *	The error code can be any of the errors returned by
 *	the open() POSIX function.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t FSNode::SetTo( const String & cPath, int nOpenMode )
{
	int nNewFD = -1;

	if( cPath.size() > 1 && cPath[0] == '~' && cPath[1] == '/' )
	{
		const char *pzHome = getenv( "HOME" );

		if( pzHome == NULL )
		{
			errno = ENOENT;
			return ( -1 );
		}
		String cRealPath = pzHome;
		cRealPath.str().insert( cRealPath.end(), cPath.begin(  ) + 1, cPath.end(  ) );
		nNewFD = open( cRealPath.c_str(), nOpenMode, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR );
	}
	else if( cPath.size() > 1 && cPath[0] == '^' && cPath[1] == '/' )
	{
		int nDir = open_image_file( IMGFILE_BIN_DIR );

		if( nDir < 0 )
		{
			return ( -1 );
		}
		nNewFD = based_open( nDir, cPath.c_str() + 2, nOpenMode );
		int nOldErr = errno;

		close( nDir );
		if( nNewFD < 0 )
		{
			errno = nOldErr;
		}
	}
	else
	{
		nNewFD = open( cPath.c_str(), nOpenMode, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR );
	}

	if( nNewFD < 0 )
	{
		return ( -1 );
	}
	struct stat sStat;

	if( fstat( nNewFD, &sStat ) < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( -1 );
	}
	status_t nError = FDChanged( nNewFD, sStat );

	if( nError < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( nError );
	}
	if( m->m_nFD >= 0 )
	{
		close( m->m_nFD );
	}
	m_sStatCache = sStat;
	m->m_nFD = nNewFD;
	return ( 0 );
}

/** Open a node using a dir/path pair.
 * \par Description:
 *	Open a node by using a directory and a path relative to
 *	that directory.
 *
 *	The path can eighter be absolute (\p cDir will then be
 *	ignored) or it can be relative to \p cDir. This have much the
 *	same semantics as setting the current working directory to \p
 *	cDir and then open the node by calling SetTo( const
 *	String& cPath, int nOpenMode ) with the path. The main
 *	advantage with this function is that it is thread-safe. You
 *	don't get any races while temporarily changing the current
 *	working directory.
 *
 *	For a more detailed description look at:
 *	SetTo( const String& cPath, int nOpenMode )
 *
 * \note
 *	If this call fail the old state of the FSNode will remain
 *	unchanged
 * \param cDir
 *	A valid directory from which the \p cPath is relative to.
 * \param cPath
 *	The file path relative to \p cDir. The path can eighter be
 *	absoulute (in which case \p cDir is ignored) or it can
 *	be relative to \p cDir.
 * \param nOpenMode
 *	Flags controlling how to open the node. See
 *	SetTo( const String& cPath, int nOpenMode )
 *	for a full description of the various flags.
 *
 * \return
 *	On success 0 is returned. On error -1 is returned and a
 *	error code is assigned to the global variable "errno".
 *	The error code can be any of the errors returned by
 *	the open() POSIX function.
 *
 * \sa FSNode( const String& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t FSNode::SetTo( const Directory & cDir, const String & cPath, int nOpenMode )
{
	if( cDir.IsValid() == false )
	{
		errno = EINVAL;
		return ( -1 );
	}
	int nNewFD = based_open( cDir.GetFileDescriptor(), cPath.c_str(  ), nOpenMode, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR );

	if( nNewFD < 0 )
	{
		return ( -1 );
	}
	struct stat sStat;

	if( fstat( nNewFD, &sStat ) < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( -1 );
	}
	status_t nError = FDChanged( nNewFD, sStat );

	if( nError < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( nError );
	}
	if( m->m_nFD >= 0 )
	{
		close( m->m_nFD );
	}
	m_sStatCache = sStat;
	m->m_nFD = nNewFD;
	return ( 0 );
}

/** Open the node referred to by the given os::FileReference.
 * \par Description:
 *	Same semantics SetTo( const String& cPath, int nOpenMode )
 *	except that the node to open is targeted by a file reference
 *	rather than a regular path.
 * \par Note:
 *	If this call fail the old state of the FSNode will remain
 *	unchanged
 * \return
 *	On success 0 is returned. On error -1 is returned and a
 *	error code is assigned to the global variable "errno".
 *	The error code can be any of the errors returned by
 *	the open() POSIX function.
 * \sa SetTo( const String& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


status_t FSNode::SetTo( const FileReference & cRef, int nOpenMode )
{
	if( cRef.IsValid() == false )
	{
		errno = EINVAL;
		return ( -1 );
	}
	int nNewFD = based_open( cRef.GetDirectory().GetFileDescriptor(  ), cRef.GetName(  ).c_str(  ), nOpenMode, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR );

	if( nNewFD < 0 )
	{
		return ( -1 );
	}
	struct stat sStat;

	if( fstat( nNewFD, &sStat ) < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( -1 );
	}
	status_t nError = FDChanged( nNewFD, sStat );

	if( nError < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( nError );
	}
	if( m->m_nFD >= 0 )
	{
		close( m->m_nFD );
	}
	m_sStatCache = sStat;
	m->m_nFD = nNewFD;
	return ( 0 );
}

/** Make the FSNode represent an already open file.
 * \par Description:
 *	Make the FSNode represent an already open file.
 * \par Note:
 *	If this call fail the old state of the FSNode will remain
 *	unchanged
 * \return
 *	On success 0 is returned. On error -1 is returned and a
 *	error code is assigned to the global variable "errno".
 *	The error code can be any of the errors returned by
 *	the open() POSIX function.
 * \since 0.3.7
 * \sa SetTo( const String& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t FSNode::SetTo( int nNewFD )
{
	struct stat sStat;

	if( fstat( nNewFD, &sStat ) < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( -1 );
	}
	status_t nError = FDChanged( nNewFD, sStat );

	if( nError < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( nError );
	}
	if( m->m_nFD >= 0 )
	{
		close( m->m_nFD );
	}
	m_sStatCache = sStat;
	m->m_nFD = nNewFD;
	return ( 0 );
}

/** Copy another FSNode
 * \par Description:
 *	Make this node a clone of \p cNode.
 * \par Note:
 *	If this call fail the old state of the FSNode will remain
 *	unchanged
 * \param cNode
 *	The FSNode to copy.
 *
 * \return
 *	On success 0 is returned. On error -1 is returned and a
 *	error code is assigned to the global variable "errno".
 *	The error code can be any of the errors returned by
 *	the open() POSIX function.
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t FSNode::SetTo( const FSNode & cNode )
{
	int nNewFD = fcntl( cNode.m->m_nFD, F_COPYFD, -1 );

	if( nNewFD < 0 )
	{
		return ( -1 );
	}
	status_t nError = FDChanged( nNewFD, cNode.m_sStatCache );

	if( nError < 0 )
	{
		int nSavedErrno = errno;

		close( nNewFD );
		errno = nSavedErrno;
		return ( nError );
	}
	if( m->m_nFD >= 0 )
	{
		close( m->m_nFD );
	}
	m_sStatCache = cNode.m_sStatCache;
	m->m_nFD = nNewFD;
	return ( 0 );
}

/** Reset the FSNode
 * \par Description:
 *	Will close the file descriptor and other resources may
 *	consumed by the FSNode. The IsValid() member will return false
 *	until the node is reinitialized with one of the SetTo()
 *	members.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void FSNode::Unset()
{
	if( m->m_hAttrDir != NULL )
	{
		close_attrdir( m->m_hAttrDir );
		m->m_hAttrDir = NULL;
	}
	FDChanged( -1, m_sStatCache );
	if( m->m_nFD != -1 )
	{
		close( m->m_nFD );
		m->m_nFD = -1;
	}
}

status_t FSNode::FDChanged( int nNewFD, const struct stat & sStat )
{
	return ( 0 );
}

status_t FSNode::GetStat( struct stat * psStat, bool bUpdateCache ) const
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( bUpdateCache )
	{
		status_t nError =::fstat( m->m_nFD, &m_sStatCache );

		if( nError < 0 )
		{
			return ( nError );
		}
	}
	if( psStat != NULL )
	{
		*psStat = m_sStatCache;
	}
	return ( 0 );
}

ino_t FSNode::GetInode() const
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	return ( m_sStatCache.st_ino );
}

dev_t FSNode::GetDev() const
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	return ( m_sStatCache.st_dev );
}

int FSNode::GetMode( bool bUpdateCache ) const
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( bUpdateCache )
	{
		if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
		{
			return ( -1 );
		}
	}
	return ( m_sStatCache.st_mode );
}

off_t FSNode::GetSize( bool bUpdateCache ) const
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( bUpdateCache )
	{
		if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
		{
			return ( -1 );
		}
	}
	return ( m_sStatCache.st_size );
}

time_t FSNode::GetCTime( bool bUpdateCache ) const
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( bUpdateCache )
	{
		if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
		{
			return ( -1 );
		}
	}
	return ( m_sStatCache.st_ctime );
}

time_t FSNode::GetMTime( bool bUpdateCache ) const
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( bUpdateCache )
	{
		if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
		{
			return ( -1 );
		}
	}
	return ( m_sStatCache.st_mtime );
}

time_t FSNode::GetATime( bool bUpdateCache ) const
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( bUpdateCache )
	{
		if( ::fstat( m->m_nFD, &m_sStatCache ) < 0 )
		{
			return ( -1 );
		}
	}
	return ( m_sStatCache.st_atime );
}

/** Read the node's attribute directory.
 * \par Description:
 *	Iterate over the node's "attribute directory". Call this
 *	member in sequence until it return "0" to iterate over all the
 *	attributes associated with the node. The attribute iterator
 *	can be reset to the first attribute with the RewindAttrdir()
 *	member.
 *
 *	More info about the returned attributes can be obtain with the
 *	StatAttr() member and the content of an attribute can be read with
 *	the ReadAttr() member.
 * \par Note:
 *	Currently only the Syllable native filesystem (AFS) support
 *	attributes so if the the file is not located on an AFS volume
 *	this member will fail.
 *
 * \param pcName
 *	Pointer to an STL string that will receive the name.
 * \return
 *	If a new name was successfully obtained 1 will be returned. If
 *	we reach the end of the attribute directory 0 will be
 *	returned.  Any other errors will cause -1 to be returned and a
 *	error code will be stored in the global "errno" variable.
 * \sa StatAttr(), ReadAttr(), WriteAttr()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t FSNode::GetNextAttrName( String * pcName )
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( m->m_hAttrDir == NULL )
	{
		m->m_hAttrDir = open_attrdir( m->m_nFD );
		if( m->m_hAttrDir == NULL )
		{
			return ( -1 );
		}
	}
	struct dirent *psEntry = read_attrdir( m->m_hAttrDir );

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

/** Reset the attribute directory iterator
 * \par Description:
 *	RewindAttrdir() will cause the next GetNextAttrName() call to
 *	return the name of the first attribute associated with this
 *	node.
 * \par Note:
 *	Currently only the Syllable native filesystem (AFS) support
 *	attributes so if the the file is not located on an AFS volume
 *	this member will fail.
 * \return
 *	0 on success. On failure -1 is returned and a error code is stored
 *	in the global variable "errno".
 * \sa GetNextAttrName()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t FSNode::RewindAttrdir()
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( m->m_hAttrDir != NULL )
	{
		rewind_attrdir( m->m_hAttrDir);
	}
	return ( 0 );
}

/** Add/update an attribute
 * \par Description:
 *	WriteAttr() is used to create new attributes and update
 *	existing attributes. A attribute is simply a chunc of data
 *	that is associated with the file but that is not part of the
 *	files regular data-stream. Attributes can be added to all
 *	kind's of FS nodes like regular files, directories, and
 *	symlinks.
 *
 *	A attribute can contain a untyped stream of data of an
 *	arbritary length or it can contain typed data like integers,
 *	floats, strings, etc etc. The reason for having typed data
 *	is to be able to make a search index that can be used to
 *	for efficient search for files based on their attributes.
 *	The indexing system is not fully implemented yet but will
 *	be part of Syllable in the future.
 *
 * \par Note:
 *	Currently only the Syllable native filesystem (AFS) support
 *	attributes so if the the file is not located on an AFS volume
 *	this member will fail.
 * \param cAttrName
 *	The name of the attribute. The name must be unique inside the
 *	scope of the node it belongs to. If an attribute already exists
 *	with that name it will be overwritten.
 * \param nFlags
 *	Currently only O_TRUNC is accepted. If you pass in O_TRUNC and a
 *	attribute with the same name already exists it will be truncated
 *	to a size of 0 before the new attribute data is written. By passing
 *	in 0 you can update parts of or extend an existing attribute.
 * \param nType
 *	The data-type of the attribute. This should be one of the ATTR_TYPE_*
 *	types defined in <atheos/filesystem.h>.
 *
 *	- \b ATTR_TYPE_NONE,	Untyped "raw" data of any size.
 *	- \b ATTR_TYPE_INT32,	32-bit integer value (the size must be exactly 4 bytes).
 *	- \b ATTR_TYPE_INT64,	64-bit integer value often used for time-values (the size must be exactly 8 bytes).
 *	- \b ATTR_TYPE_FLOAT,	32-bit floating point value (the size must be exactly 4 bytes).
 *	- \b ATTR_TYPE_DOUBLE,	64-bit floating point value (the size must be exactly 8 bytes).
 *	- \b ATTR_TYPE_STRING,	UTF8 string. The string should not be NUL terminated.
 * \param pBuffer
 *	Pointer to the data to be written.
 * \param nPos
 *	The offset into the attribute where the data will be written.
 * \param nLen
 *	Number of bytes to be written.
 *
 * \return
 *	On success the number of bytes actually written is
 *	returned. On failure -1 is returned and the error code is
 *	stored in the global variable "errno"
 * 
 * \sa ReadAttr(), StatAttr()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

ssize_t FSNode::WriteAttr( const String & cAttrName, int nFlags, int nType, const void *pBuffer, off_t nPos, size_t nLen )
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	return ( write_attr( m->m_nFD, cAttrName.c_str(), nFlags, nType, pBuffer, nPos, nLen ) );
}

/** Read the data held by an attribute.
 * \par Description:
 *	Read an arbritary chunk of an attributes data. Both the name
 *	and the type must match for the operation so succede. If you
 *	don't know the type in advance it can be retrieved with the
 *	StatAttr() member.
 * 
 * \par Note:
 *	Currently only the Syllable native filesystem (AFS) support
 *	attributes so if the the file is not located on an AFS volume
 *	this member will fail.
 * \param cAttrName
 *	The name of the attribute to read.
 * \param nType
 *	The expected attribute type. The attribute must be of this type
 *	for the function to succede. See WriteAttr() for a more detailed
 *	descritpion of attribute types.
 *
 * \return
 *	On success the number of bytes actually read is returned. On
 *	failure -1 is returned and the error code is stored in the
 *	global variable "errno".
 * 
 * \sa WriteAttr(), StatAttr()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


ssize_t FSNode::ReadAttr( const String & cAttrName, int nType, void *pBuffer, off_t nPos, size_t nLen )
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	return ( read_attr( m->m_nFD, cAttrName.c_str(), nType, pBuffer, nPos, nLen ) );

}

/** Remove an attribute from an FS node.
 * \par Description:
 *	This will remove the named attribute from the node itself and
 *	if the attribute has been indexed it will also be removed from
 *	the index.
 * \par Note:
 *	Currently only the Syllable native filesystem (AFS) support
 *	attributes so if the the file is not located on an AFS volume
 *	this member will fail.
 * \param cName
 *	Name of the attribute to remove.
 * \return
 *	On success 0 is returned. On failure -1 is returned and the
 *	error code is stored in the global variable "errno".
 * \sa WriteAttr(), ReadAttr()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


status_t FSNode::RemoveAttr( const String & cName )
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	return ( remove_attr( m->m_nFD, cName.c_str() ) );
}

/** Get extended info about an attribute.
 * \par Description:
 *	Call this function to retrieve the size and type of an
 *	attribute.  For a detailed description of the attribute type
 *	take a look at WriteAttr().
 * \par Note:
 *	Currently only the Syllable native filesystem (AFS) support
 *	attributes so if the the file is not located on an AFS volume
 *	this member will fail.
 * \param cName
 *	The name of the attribute to examine.
 * \return
 *	On success 0 is returned. On failure -1 is returned and the
 *	error code is stored in the global variable "errno".
 * \sa WriteAttr(), ReadAttr()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t FSNode::StatAttr( const String & cName, struct attr_info *psBuffer )
{
	if( m->m_nFD < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	return ( stat_attr( m->m_nFD, cName.c_str(), psBuffer ) );
}

int FSNode::GetFileDescriptor() const
{
	return ( m->m_nFD );
}



