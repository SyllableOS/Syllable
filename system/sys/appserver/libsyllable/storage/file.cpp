
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
#include <limits.h>
#include <storage/file.h>
#include <storage/filereference.h>
#include <storage/path.h>

#include <util/exceptions.h>

#include <new>

using namespace os;

class File::Private
{
	public:
	Private():m_cMutex( "os::File" ) {
		m_pBuffer = NULL;
		m_nBufferSize = DEFAULT_BUFFER_SIZE;
		m_nValidBufSize = 0;
		m_nPosition = 0;
		m_nBufPos = 0;
		m_bDirty = false;
	}
	
	~Private() {
	}

    mutable Locker m_cMutex;
    uint8* m_pBuffer;
    int	   m_nBufferSize;
    int	   m_nValidBufSize;
    off_t  m_nPosition;
    off_t  m_nBufPos;
    bool   m_bDirty;
};

void File::_Init()
{
	m = new Private;
}

/** Default constructor
 * \par Description:
 *	Initialize the instance to a known but invalid state. The instance
 *	must be successfully initialized with one of the SetTo() members
 *	before it can be used.
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

File::File()
{
	_Init();
}

/** Construct a file from a regular path.
 * \par Description:
 *	Open the file pointed to by \p cPath. The node must be a regular file
 *	or a symlink pointing at a regular file. Anything else will cause and
 *	errno_exception(EINVAL) exception to be thrown.
 *
 * \param cPath
 *	The file to open. The path can eigther be absolute (starting
 *	with "/") or relative to the current working directory.
 * \param nOpenMode
 
 *	Flags controling how the file should be opened. The value
 *	should be one of the O_RDONLY, O_WRONLY, or O_RDWR. In addition
 *	the following flags can be bitwise or'd in to further control
 *	the operation:
 *
 *	- O_TRUNC	trunate the size to 0.
 *	- O_APPEND	automatically move the file-pointer to the end
 *			of the file before each write (only valid for files)
 *	- O_NONBLOCK	open the file in non-blocking mode. This is only
 *			relevant for named pipes and PTY's.
 *	- O_NOCTTY	Don't make the file controlling TTY even if \p cPath
 *			points to a PTY.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

File::File( const String & cPath, int nOpenMode ):FSNode( cPath, nOpenMode & ~O_NOTRAVERSE )
{
	_Init();

	if( S_ISDIR( GetMode( false ) ) )
	{
		throw errno_exception( "Node not a file", EINVAL );
	}
}

/** Open a file addressed as a name inside a specified directory.
 * \par Description:
 *	Look at File( const std::string& cPath, int nOpenMode ) for a more
 *	detailed description.
 *
 * \param cDir
 *	The directory to use as a base for \p cPath
 * \param cPath
 *	Path relative to \p cPath.
 * \param nOpenMode
 *	Flags controlling how to open the file. See
 *	File( const std::string& cPath, int nOpenMode )
 *	for a full description.
 *
 * \sa File( const std::string& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


File::File( const Directory & cDir, const String & cPath, int nOpenMode ):FSNode( cDir, cPath, nOpenMode & ~O_NOTRAVERSE )
{
	_Init();

	if( S_ISDIR( GetMode( false ) ) )
	{
		throw errno_exception( "Node not a file", EINVAL );
	}
}

/** Open a file referred to by a os::FileReference.
 * \par Description:
 *	Look at File( const std::string& cPath, int nOpenMode ) for a more
 *	detailed description.
 *
 * \param cRef
 *	Reference to the file to open.
 * \param nOpenMode
 *	Flags controlling how to open the file. See
 *	File( const std::string& cPath, int nOpenMode )
 *	for a full description.
 *
 * \sa File( const std::string& cPath, int nOpenMode )
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

File::File( const FileReference & cRef, int nOpenMode ):FSNode( cRef, nOpenMode & ~O_NOTRAVERSE )
{
	_Init();
	if( S_ISDIR( GetMode( false ) ) )
	{
		throw errno_exception( "Node not a file", EINVAL );
	}
}

/** Construct a file from a FSNode.
 * \par Description:
 *	This constructor can be used to "downcast" an os::FSNode to a
 *	os::File. The FSNode must represent a regular file, pipe, or PTY.
 *	Attempts to convert symlinks and directories will cause an
 *	errno_exception(EINVAL) exception to be thrown.
 *
 * \param cNode
 *	The FSNode to downcast.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

File::File( const FSNode & cNode ):FSNode( cNode )
{
	_Init();

	if( S_ISDIR( GetMode( false ) ) || S_ISLNK( GetMode( false ) ) )
	{
		throw errno_exception( "Node not a file", EINVAL );
	}
}

/** Construct a file object from a open filedescriptor.
 * \par Description:
 *	Construct a file object from a open filedescriptor.
 * \note
 *	The file descriptor will be close when the object is deleted.
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \since 0.3.7
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


File::File( int nFD ):FSNode( nFD )
{
	_Init();

	if( S_ISDIR( GetMode( false ) ) )
	{
		throw errno_exception( "Node not a file", EINVAL );
	}
}

/** Copy constructor.
 * \par Description:
 *	This constructor will make an independent copy of \p cFile.
 *	The copy and original will have their own file pointers and
 *	they will have their own attribute directory iterators (see note).
 * \par Note:
 *	The attribute directory iterator will not be cloned so when
 *	FSNode::GetNextAttrName() is called for the first time it will
 *	return the first attribute name even if the iterator was not
 *	located at the beginning of the originals attribute directory.
 *
 * \param cFile
 *	The file object to copy.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

File::File( const File & cFile ):FSNode( cFile )
{
	_Init();
	m->m_nPosition = cFile.m->m_nPosition;
	m->m_nBufferSize = cFile.m->m_nBufferSize;
}

File::~File()
{
	if( m->m_bDirty )
	{
		Flush();
	}
	delete[]m->m_pBuffer;
	delete m;
}

status_t File::FDChanged( int nNewFD, const struct stat & sStat )
{
	if( m->m_bDirty && IsValid() )
	{
		Flush();
	}
	m->m_nValidBufSize = 0;
	return ( 0 );
}

status_t File::_FillBuffer( off_t nPos )
{
	if( m->m_bDirty )
	{
		if( Flush() < 0 )
		{
			return ( -1 );
		}
	}
	m->m_nBufPos = nPos;

	while( true )
	{
		ssize_t nLen = read_pos( GetFileDescriptor(), nPos, m->m_pBuffer, m->m_nBufferSize );

		if( nLen < 0 )
		{
			if( errno == EINTR )
			{
				continue;
			}
			else
			{
				return ( -1 );
			}
		}
		m->m_nValidBufSize = nLen;
		return ( 0 );
	}
}

/** Set the size of the files caching buffer.
 * \par Description:
 *	By default the os::File class use a 32KB internal memory
 *	buffer to cache resently read/written data. Normally you the
 *	buffer can greatly increase the performance since it reduce
 *	the number of kernel-calls when doing multiple small reads or
 *	writes on the file. There is cases however where it is
 *	beneficial to increase the buffer size or even to disabling
 *	buffering entirely.
 *
 *	If you read/write large chunks of data at the time the buffer
 *	might impose more overhead than gain and it could be a good
 *	idea to disable the buffering entirely by setting the buffer
 *	size to 0. When for example streaming video the amount of data
 *	read are probably going to be much larger than the buffer
 *	anyway and each byte is only read once by the application and
 *	if the application read the file in reasonably sized chunks
 *	the extra copy imposed by reading the data into the internal
 *	buffer and then copy it to the callers buffer will only
 *	decrease the performance.
 *
 * \param nBufferSize
 *	The buffer size in bytes. If the size is set to 0 the file
 *	will be unbuffered and each call to Read()/Write() and
 *	ReadPos()/WritePos() will translate directly to kernel calls.
 * 
 * \return
 *	On success 0 is returned. If there was not enough memory for
 *	the new buffer -1 will be returned and the global variable
 *	"errno" will be set to ENOMEM.
 * \sa GetBufferSize()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t File::SetBufferSize( int nBufferSize )
{
	AutoLocker cLock( &m->m_cMutex );

	if( nBufferSize == m->m_nBufferSize )
	{
		return ( 0 );
	}
	if( nBufferSize == 0 )
	{
		if( m->m_bDirty )
		{
			if( Flush() < 0 )
			{
				return ( -1 );
			}
		}
		m->m_nValidBufSize = 0;
		m->m_nBufferSize = 0;
		delete[]m->m_pBuffer;
		m->m_pBuffer = NULL;
	}
	else
	{
		try
		{
			if( m->m_pBuffer != NULL )
			{
				uint8 *pNewBuffer = new uint8[nBufferSize];

				if( m->m_nValidBufSize > nBufferSize )
				{
					m->m_nValidBufSize = nBufferSize;
				}
				if( m->m_nValidBufSize > 0 )
				{
					memcpy( pNewBuffer, m->m_pBuffer, m->m_nValidBufSize );
				}
				delete[]m->m_pBuffer;
				m->m_pBuffer = pNewBuffer;
			}
			m->m_nBufferSize = nBufferSize;
		}
		catch( std::bad_alloc & cExc )
		{
			errno = ENOMEM;
			return ( -1 );
		}
	}
	return ( 0 );
}

/** Obtain the files buffer size.
 * \return
 *	The files buffer size in bytes. A value of 0 means the file is
 *	unbuffered.
 * \sa SetBufferSize()
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int File::GetBufferSize() const
{
	return ( m->m_nBufferSize );
}

/** Write unwritten data to the underlying file.
 * \par Note:
 *	Flush() will be called automatically by the destructor if there is
 *	unwritten data in the buffer.
 * \return
 *	On success 0 will be returned. On failure -1 will be returned and
 *	a error code will be stored in the global variable "errno".
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t File::Flush()
{
	AutoLocker cLock( &m->m_cMutex );

	if( m->m_bDirty )
	{
		ssize_t nSize = m->m_nValidBufSize;
		int nOffset = 0;

		while( nSize > 0 )
		{
			ssize_t nLen = write_pos( GetFileDescriptor(), m->m_nBufPos + nOffset, m->m_pBuffer + nOffset, nSize );

			if( nLen < 0 )
			{
				if( errno == EINTR )
				{
					continue;
				}
				else
				{
					return ( -1 );
				}
			}
			nSize -= nLen;
			nOffset += nLen;
		}
		m->m_bDirty = false;
	}
	return ( 0 );
}

ssize_t File::Read( void *pBuffer, ssize_t nSize )
{
	AutoLocker cLock( &m->m_cMutex );
	ssize_t nLen = ReadPos( m->m_nPosition, pBuffer, nSize );

	if( nLen > 0 )
	{
		m->m_nPosition += nLen;
	}
	return ( nLen );
}

ssize_t File::Write( const void *pBuffer, ssize_t nSize )
{
	AutoLocker cLock( &m->m_cMutex );
	ssize_t nLen = WritePos( m->m_nPosition, pBuffer, nSize );

	if( nLen > 0 )
	{
		m->m_nPosition += nLen;
	}
	return ( nLen );
}

ssize_t File::ReadPos( off_t nPos, void *pBuffer, ssize_t nSize )
{
	AutoLocker cLock( &m->m_cMutex );

	if( m->m_nBufferSize == 0 )
	{
		return ( read_pos( GetFileDescriptor(), nPos, pBuffer, nSize ) );
	}
	if( m->m_pBuffer == NULL )
	{
		try
		{
			m->m_pBuffer = new uint8[m->m_nBufferSize];
		} catch( std::bad_alloc & cExc )
		{
			errno = ENOMEM;
			return ( -1 );
		}
	}
	ssize_t nBytesRead = 0;

	if( nSize > m->m_nBufferSize * 2 )
	{
		ssize_t nPreSize = nSize - m->m_nBufferSize;

		if( m->m_nValidBufSize == 0 || nPos + nPreSize <= m->m_nBufPos || nPos > m->m_nBufPos + m->m_nValidBufSize )
		{
			ssize_t nLen = read_pos( GetFileDescriptor(), nPos, pBuffer, nPreSize );

			if( nLen < 0 )
			{
				return ( nLen );
			}
			nBytesRead += nLen;
			nPos += nLen;
			nSize -= nLen;
			pBuffer = ( ( uint8 * )pBuffer ) + nLen;
		}
	}

	while( nSize > 0 )
	{
		if( nPos < m->m_nBufPos || nPos >= m->m_nBufPos + m->m_nValidBufSize )
		{
			if( _FillBuffer( nPos ) < 0 )
			{
				return ( ( nBytesRead > 0 ) ? nBytesRead : -1 );
			}
			else if( m->m_nValidBufSize == 0 )
			{
				return ( nBytesRead );
			}
		}
		int nBufOffset = nPos - m->m_nBufPos;
		ssize_t nCurLen = m->m_nValidBufSize - nBufOffset;

		if( nCurLen > nSize )
		{
			nCurLen = nSize;
		}
		memcpy( pBuffer, m->m_pBuffer + nBufOffset, nCurLen );
		nBytesRead += nCurLen;
		nPos += nCurLen;
		nSize -= nCurLen;
		pBuffer = ( ( uint8 * )pBuffer ) + nCurLen;
	}
	return ( nBytesRead );
}

ssize_t File::WritePos( off_t nPos, const void *pBuffer, ssize_t nSize )
{
	AutoLocker cLock( &m->m_cMutex );

	if( m->m_nBufferSize == 0 )
	{
		return ( write_pos( GetFileDescriptor(), nPos, pBuffer, nSize ) );
	}
	if( m->m_pBuffer == NULL )
	{
		try
		{
			m->m_pBuffer = new uint8[m->m_nBufferSize];
		} catch( std::bad_alloc & cExc )
		{
			errno = ENOMEM;
			return ( -1 );
		}
	}
	ssize_t nBytesWritten = 0;

	if( nSize > m->m_nBufferSize * 2 )
	{
		ssize_t nPreSize = nSize - m->m_nBufferSize;

		if( m->m_nValidBufSize == 0 || nPos + nPreSize <= m->m_nBufPos || nPos > m->m_nBufPos + m->m_nValidBufSize )
		{
			ssize_t nLen = write_pos( GetFileDescriptor(), nPos, pBuffer, nPreSize );

			if( nLen < 0 )
			{
				return ( nLen );
			}
			nBytesWritten += nLen;
			nPos += nLen;
			nSize -= nLen;
			pBuffer = ( ( const uint8 * )pBuffer ) + nLen;
		}
	}
	if( nPos < m->m_nBufPos || nPos > m->m_nBufPos + m->m_nValidBufSize )
	{
		if( m->m_bDirty )
		{
			if( Flush() < 0 )
			{
				return ( -1 );
			}
		}
		m->m_nBufPos = nPos;
		m->m_nValidBufSize = 0;
	}
	while( nSize > 0 )
	{
		if( nPos >= m->m_nBufPos + m->m_nBufferSize )
		{
			if( m->m_bDirty )
			{
				if( Flush() < 0 )
				{
					return ( -1 );
				}
			}
			m->m_nBufPos = nPos;
			m->m_nValidBufSize = 0;
		}
		int nBufOffset = nPos - m->m_nBufPos;
		ssize_t nCurLen = m->m_nBufferSize - nBufOffset;

		if( nCurLen > nSize )
		{
			nCurLen = nSize;
		}
		memcpy( m->m_pBuffer + nBufOffset, pBuffer, nCurLen );
		if( nBufOffset + nCurLen > m->m_nValidBufSize )
		{
			m->m_nValidBufSize = nBufOffset + nCurLen;
		}
		nBytesWritten += nCurLen;
		nPos += nCurLen;
		nSize -= nCurLen;
		pBuffer = ( ( const uint8 * )pBuffer ) + nCurLen;
		m->m_bDirty = true;
	}
	return ( nBytesWritten );
}

off_t File::GetSize( bool bUpdateCache ) const
{
	AutoLocker cLock( &m->m_cMutex );
	off_t nSize = FSNode::GetSize( bUpdateCache );

	if( nSize != -1 && m->m_nValidBufSize > 0 && m->m_nBufPos + m->m_nValidBufSize > nSize )
	{
		nSize = m->m_nBufPos + m->m_nValidBufSize;
	}
	return ( nSize );
}

/** Move the file pointer.
 * \par Description:
 * \par Note:
 * \par Warning:
 * \param
 * \return
 * \par Error codes:
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


off_t File::Seek( off_t nPos, int nMode )
{
	AutoLocker cLock( &m->m_cMutex );

	switch ( nMode )
	{
	case SEEK_SET:
		if( nPos < 0 )
		{
			errno = EINVAL;
			return ( -1 );
		}
		m->m_nPosition = nPos;
		return ( m->m_nPosition );
	case SEEK_CUR:
		if( m->m_nPosition + nPos < 0 )
		{
			errno = EINVAL;
			return ( -1 );
		}
		m->m_nPosition += nPos;
		return ( m->m_nPosition );
	case SEEK_END:
		{
			off_t nSize = GetSize();

			if( nSize + nPos < 0 )
			{
				errno = EINVAL;
				return ( -1 );
			}
			m->m_nPosition = nSize + nPos;
			return ( m->m_nPosition );
		}
	default:
		errno = EINVAL;
		return ( -1 );
	}
}
