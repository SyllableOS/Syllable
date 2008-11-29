
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

#include <errno.h>
#include <atheos/elf.h>
#include <util/resources.h>

#include <vector>

using namespace os;


#define RES_HDR_SIZE	(sizeof(int)   /* magic   */ + \
			 sizeof(int)   /* version */ + \
			 sizeof(off_t) /* size    */ + \
			 sizeof(int)   /* count   */ )


/** \internal */

struct ResourceDesc
{
	off_t m_nFileOffset;
	ssize_t m_nHeaderSize;
	ssize_t m_nSize;
	String m_cName;
	String m_cType;
};

/** \internal */

class Resources::Private
{
	public:
	~Private();
	SeekableIO * m_pcStream;
	off_t m_nResOffset;
	off_t m_nTotalSize;
	bool m_bReadOnly;
	bool m_bDeleteStream;
	std::vector < ResourceDesc* > m_apResources;
};

class ResStream::Private
{
	public:
	SeekableIO * m_pcStream;
	off_t m_nImageOffset;
	ssize_t m_nSize;
	String m_cName;
	String m_cType;
	off_t m_nCurOffset;
	bool m_bReadOnly;
};

Resources::Private::~Private()
{
	/* Delete the ResourceDescs from m_apResources */
	while( !m_apResources.empty() ) {
		delete( m_apResources.back() );
		m_apResources.pop_back();
	}
}

ResStream::ResStream( SeekableIO * pcFile, off_t nOffset, const String & cName, const String & cType, ssize_t nSize, bool bReadOnly )
{
	m = new Private;

	m->m_pcStream = pcFile;
	m->m_cName = cName;
	m->m_cType = cType;
	m->m_nImageOffset = nOffset;
	m->m_nSize = nSize;
	m->m_nCurOffset = 0LL;
	m->m_bReadOnly = bReadOnly;
}

ResStream::~ResStream()
{
	delete m;
}

/** Get the resource name.
 * \par Description:
 *	Get the resource name.
 * \return An STL string containing the resource name.
 * \sa GetType(), GetSize()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

String ResStream::GetName() const
{
	return ( m->m_cName );
}

/** Get the resource mime-type.
 * \par Description:
 *	Get the resource mime-type.
 * \return An STL string containing the resource mime-type.
 * \sa GetName(), GetSize()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

String ResStream::GetType() const
{
	return ( m->m_cType );
}

/** Get the size of the resource.
 * \par Description:
 *	Get the size of the resource.
 * \return The resource size in bytes.
 * \sa GetName(), GetType()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

ssize_t ResStream::GetSize() const
{
	return ( m->m_nSize );
}

ssize_t ResStream::Read( void *pBuffer, ssize_t nSize )
{
	if( m->m_nCurOffset + nSize > m->m_nSize )
	{
		nSize = m->m_nSize - m->m_nCurOffset;
	}
	if( nSize > 0 )
	{
		nSize = m->m_pcStream->ReadPos( m->m_nImageOffset + m->m_nCurOffset, pBuffer, nSize );
		if( nSize > 0 )
		{
			m->m_nCurOffset += nSize;
		}
	}
	return ( nSize );
}

ssize_t ResStream::Write( const void *pBuffer, ssize_t nSize )
{
	if( m->m_nCurOffset + nSize > m->m_nSize )
	{
		nSize = m->m_nSize - m->m_nCurOffset;
	}
	if( nSize > 0 )
	{
		nSize = m->m_pcStream->WritePos( m->m_nImageOffset + m->m_nCurOffset, pBuffer, nSize );
		if( nSize > 0 )
		{
			m->m_nCurOffset += nSize;
		}
	}
	return ( nSize );
}

ssize_t ResStream::ReadPos( off_t nPos, void *pBuffer, ssize_t nSize )
{
	if( nPos < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( nPos >= m->m_nSize )
	{
		return ( 0 );
	}
	if( nPos + nSize > m->m_nSize )
	{
		nSize = m->m_nSize - nPos;
	}
	nSize = m->m_pcStream->ReadPos( m->m_nImageOffset + nPos, pBuffer, nSize );
	return ( nSize );
}

ssize_t ResStream::WritePos( off_t nPos, const void *pBuffer, ssize_t nSize )
{
	if( nPos < 0 )
	{
		errno = EINVAL;
		return ( -1 );
	}
	if( nPos >= m->m_nSize )
	{
		return ( 0 );
	}
	if( nPos + nSize > m->m_nSize )
	{
		nSize = m->m_nSize - nPos;
	}
	nSize = m->m_pcStream->WritePos( m->m_nImageOffset + nPos, pBuffer, nSize );
	return ( nSize );
}

off_t ResStream::Seek( off_t nPos, int nMode )
{
	switch ( nMode )
	{
	case SEEK_SET:
		m->m_nCurOffset = nPos;
		break;
	case SEEK_CUR:
		m->m_nCurOffset += nPos;
		break;
	case SEEK_END:
		m->m_nCurOffset = m->m_nSize + nPos;
		break;
	default:
		break;
	}
	if( m->m_nCurOffset < 0 )
	{
		m->m_nCurOffset = 0;
	}
	else if( m->m_nCurOffset > m->m_nSize )
	{
		m->m_nCurOffset = m->m_nSize;
	}
	return ( m->m_nCurOffset );
}


/** Construct an os::Resources object from a seekable data stream.
 * \par Description:
 *	This constructor will initialize a read-only reasorce archive from
 *	the given data-stream. The stream must point to a plain resource
 *	file or an AtheOS executable/DLL image containing a resource section.
 * \par
 *	If any errors including invalid file-format/missing resource section
 *	an os::errno_exception will be thrown.
 * \note
 *	The \p pcFile object will be deleted by the os::Resources when
 *	the os::Resources object is deleted unless it is manually
 *	detached by calling the DetachStream() member.
 * \param pcFile
 *	A readable stream referencing a valid resource archive or a valid
 *	AtheOS executable/DLL with a resource section.This stream will be
 *	deleted by the archive unless an exception is thrown from this
 *	constructor or it is detached from the archive with DetachStream().
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Resources::Resources( SeekableIO * pcFile )
{
	char anBuffer[4];
	int nError = pcFile->Read( anBuffer, 4 );

	if( nError != 4 )
	{
		if( nError < 0 )
		{
			throw( errno_exception( "failed to read resource file" ) );
		}
		else
		{
			throw( errno_exception( "invalid resource file", ENOEXEC ) );
		}
	}
	if( *( ( int * )anBuffer ) == Resources::RES_MAGIC )
	{
		_Init( pcFile, 0LL, false );
	}
	else if( anBuffer[0] == ELFMAG0 && anBuffer[1] == ELFMAG1 && anBuffer[2] == ELFMAG2 && anBuffer[3] == ELFMAG3 )
	{
		off_t nOffset;
		ssize_t nSize;

		if( FindExecutableResource( pcFile, &nOffset, &nSize ) < 0 )
		{
			throw errno_exception( "could not find resource section" );
		}
		_Init( pcFile, nOffset, false );
	}
	else
	{
		throw( errno_exception( "invalid resource file", ENOEXEC ) );
	}
}

/** Construct a os::Resources from a executable or DLL image ID.
 * \par Description:
 *	This is normally the most convenient constructor to use when loading
 *	resources from the application's executable or from one of it's
 *	loaded DLL's. The constructor will obtain a file descriptor to the
 *	specified executable image using the open_image_file() syscall and
 *	then initiate the archive from this.
 * \par
 *	The ID to pass as \p nImageID can be obtained with the get_image_id()
 *	syscall. get_image_id() will return the image ID of the DLL or
 *	executable in which the calling function live.
 * \par
 *	If anything goes wrong an os::errno_exception exception will be thrown.
 * \param nImageID
 *	A valid handle to a DLL or executable loaded by this process.
 * \sa get_image_id(), open_image_file()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


Resources::Resources( int nImageID )
{
	File *pcFile = new File( open_image_file( nImageID ) );
	off_t nOffset;
	ssize_t nSize;

	if( FindExecutableResource( pcFile, &nOffset, &nSize ) < 0 )
	{
		delete pcFile;
		throw errno_exception( "could not find resource section" );
	}
	try
	{
		_Init( pcFile, nOffset, false );
	}
	catch( ... )
	{
		delete pcFile;

		throw;
	}
}

/** Construct a os::Resources object from a seekable data stream.
 * \par Description:
 *	This is the most flexible but least used constructor. It allow
 *	you to specify what position within the stream the resource
 *	archive lives and it can also initiate the archive for writing.
 * \par
 *	If the \p bCreate parameter is true a resource archive header
 *	will be written to the file and you can add resources to
 *	it with the CreateResource() member. All old resources will
 *	be deleted when creating an archive.
 * \par
 *	If anything goes wrong an os::errno_exception exception will be thrown.
 * \note
 *	The \p pcFile object will be deleted by the os::Resources when
 *	the os::Resources object is deleted unless it is manually
 *	detached by calling the DetachStream() member.
 * \param pcFile
 *	The stream to operate on. This can be an empty file if the \p bCreate
 *	argument is true. This stream will be deleted by the archive unless
 *	an exception is thrown from this constructor or it is detached
 *	from the archive with DetachStream().
 * \param nResOffset
 *	The file-position where the resource archive lives or where a new
 *	resource archive should be written.
 * \param bCreate
 *	If \b true a new archive header will be written at \p nResOffset
 *	and new resources can be added with CreateResource().
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Resources::Resources( SeekableIO * pcFile, off_t nResOffset, bool bCreate )
{
	_Init( pcFile, nResOffset, bCreate );
}

void Resources::_Init( SeekableIO * pcFile, off_t nResOffset, bool bCreate )
{
	m = new Private;

	m->m_pcStream = pcFile;
	m->m_nResOffset = nResOffset;
	m->m_bReadOnly = !bCreate;
	m->m_nTotalSize = RES_HDR_SIZE;
	m->m_bDeleteStream = true;

	if( bCreate == false )
	{
		if( _LoadResourceList() < 0 )
		{
			delete m;

			throw( errno_exception( "failed to open resources" ) );
		}
	}
	else
	{
		m->m_pcStream->Seek( nResOffset, SEEK_SET );

		int nValue = RES_MAGIC;
		if( m->m_pcStream->Write( &nValue, sizeof( int ) ) != sizeof( int ) )
		{
			delete m;

			throw( errno_exception( "failed to write resource header" ) );
		}
		nValue = RES_VERSION;
		if( m->m_pcStream->Write( &nValue, sizeof( int ) ) != sizeof( int ) )
		{
			delete m;

			throw( errno_exception( "failed to write resource header" ) );
		}
		if( m->m_pcStream->Write( &m->m_nTotalSize, sizeof( off_t ) ) != sizeof( off_t ) )
		{
			delete m;

			throw( errno_exception( "failed to write resource header" ) );
		}
		nValue = 0;
		if( m->m_pcStream->Write( &nValue, sizeof( int ) ) != sizeof( int ) )
		{
			delete m;

			throw( errno_exception( "failed to write resource header" ) );
		}
	}
}

Resources::~Resources()
{
	delete m;
}

/** Detach the data-stream to avoid it being deleted by the constructor.
 * \par Description:
 *	Normally the os::Resources object will delete the os::SeekableIO
 *	object passed in to any of the constructors when the object is
 *	deleted. If you don't want this to happen (for example because
 *	the stream-object is allocated on the stack) you must call
 *	DetachStream() before deleting the os::Resources object.
 * \note
 *	Calling any members on the os::Resources object or any of the
 *	os::ResStream objects spawned from it might still use the
 *	passed in data-stream so it is very important that you don't
 *	delete the stream yourself before you are done using the
 *	os::Resources object or any of it childrens even if you have
 *	detached the stream.
 * \par
 *	It is however safe to delete the os::Resources object after
 *	the stream so no precautions are needed to assure that the
 *	os::Resources object goes away after the os::SeekableIO in
 *	case they are both allocated on the stack. Just make sure
 *	to not call any other members after the stream is gone.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Resources::DetachStream()
{
	m->m_bDeleteStream = false;
}

status_t Resources::_LoadResourceList()
{
	m->m_pcStream->Seek( m->m_nResOffset, SEEK_SET );

	int nValue;
	int nError = m->m_pcStream->Read( &nValue, sizeof( int ) );
	if( nError != sizeof( int ) )
	{
		if( nError >= 0 )
		{
			errno = ENOEXEC;
		}
		return ( -1 );
	}
	if( nValue != RES_MAGIC )
	{
		errno = ENOEXEC;
		return ( -1 );
	}
	nError = m->m_pcStream->Read( &nValue, sizeof( int ) );
	if( nError != sizeof( int ) )
	{
		if( nError >= 0 )
		{
			errno = ENOEXEC;
		}
		return ( -1 );
	}
	if( nValue != RES_VERSION )
	{
		errno = ENOEXEC;
		return ( -1 );
	}
	off_t nTotalSize;
	nError = m->m_pcStream->Read( &nTotalSize, sizeof( off_t ) );
	if( nError != sizeof( off_t ) )
	{
		if( nError >= 0 )
		{
			errno = ENOEXEC;
		}
		return ( -1 );
	}
	int nCount;
	nError = m->m_pcStream->Read( &nCount, sizeof( int ) );
	if( nError != sizeof( int ) )
	{
		if( nError >= 0 )
		{
			errno = ENOEXEC;
		}
		return ( -1 );
	}
	for( int i = 0; i < nCount; ++i )
	{
		ResourceDesc sDesc;

		sDesc.m_nFileOffset = m->m_nResOffset + m->m_nTotalSize;
		sDesc.m_nHeaderSize = sizeof( int );	// size

		nError = m->m_pcStream->Read( &sDesc.m_nSize, sizeof( int ) );
		if( nError != sizeof( int ) )
		{
			if( nError >= 0 )
			{
				errno = ENOEXEC;
			}
			return ( -1 );
		}
		uint8 nNameLen;

		nError = m->m_pcStream->Read( &nNameLen, 1 );
		if( nError != 1 )
		{
			if( nError >= 0 )
			{
				errno = ENOEXEC;
			}
			return ( -1 );
		}
		{
			char *pzBfr = new char[ nNameLen + 1 ];
			nError = m->m_pcStream->Read( pzBfr, nNameLen );
			pzBfr[ nNameLen ] = 0;
			sDesc.m_cName = String( pzBfr, nNameLen );
			delete []pzBfr;
		}
		sDesc.m_nHeaderSize += nNameLen + 1;
		if( nError != nNameLen )
		{
			if( nError >= 0 )
			{
				errno = ENOEXEC;
			}
			return ( -1 );
		}
		nError = m->m_pcStream->Read( &nNameLen, 1 );
		if( nError != 1 )
		{
			if( nError >= 0 )
			{
				errno = ENOEXEC;
			}
			return ( -1 );
		}
		{
			char *pzBfr = new char[ nNameLen + 1 ];
			nError = m->m_pcStream->Read( pzBfr, nNameLen );
			pzBfr[ nNameLen ] = 0;
			sDesc.m_cType = String( pzBfr, nNameLen );
			delete []pzBfr;
		}
		if( nError != nNameLen )
		{
			if( nError >= 0 )
			{
				errno = ENOEXEC;
			}
			return ( -1 );
		}
		sDesc.m_nHeaderSize += nNameLen + 1;
		ResourceDesc *psDesc = new ResourceDesc( sDesc );
		m->m_apResources.push_back( psDesc );
		m->m_nTotalSize += sDesc.m_nHeaderSize + sDesc.m_nSize;
		m->m_pcStream->Seek( sDesc.m_nSize, SEEK_CUR );
	}
	if( nTotalSize != m->m_nTotalSize )
	{
		errno = ENOEXEC;
		return ( -1 );
	}
	else
	{
		return ( 0 );
	}
}

/** Get the number of resources embedded in this archive.
 * \par Description:
 *	Get the number of resources embedded in this archive.
 * \return Number of resources embedded in the archive.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int Resources::GetResourceCount() const
{
	return ( m->m_apResources.size() );
}

/** Get the name of a specified resource.
 * \par Description:
 *	Get the name of a specified resource.
 * \param nIndex
 *	Which resource to query.
 * \return The name of the specified resource.
 * \sa GetResourceType(), GetResourceSize(), GetResourceStream(), GetResourceCount()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

String Resources::GetResourceName( uint nIndex ) const
{
	if( nIndex < m->m_apResources.size() )
	{
		return ( m->m_apResources[nIndex]->m_cName );
	}
	else
	{
		errno = EINVAL;
		return ( "" );
	}
}

/** Get the mime-type of a specified resource.
 * \par Description:
 *	Get the mime-type of a specified resource.
 * \param nIndex
 *	Which resource to query.
 * \return The mime-type of the specified resource.
 * \sa GetResourceName(), GetResourceSize(), GetResourceStream(), GetResourceCount()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

String Resources::GetResourceType( uint nIndex ) const
{
	if( nIndex < m->m_apResources.size() )
	{
		return ( m->m_apResources[nIndex]->m_cType );
	}
	else
	{
		errno = EINVAL;
		return ( "" );
	}
}

/** Get the size of a specified resource.
 * \par Description:
 *	Get the size of a specified resource.
 * \param nIndex
 *	Which resource to query.
 * \return The size in bytes of the specified resource.
 * \sa GetResourceName(), GetResourceType(), GetResourceStream(), GetResourceCount()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

ssize_t Resources::GetResourceSize( uint nIndex ) const
{
	if( nIndex < m->m_apResources.size() )
	{
		return ( m->m_apResources[nIndex]->m_nSize );
	}
	else
	{
		errno = EINVAL;
		return ( -1 );
	}
}

/** Read data from a named resource.
 * \par Description:
 *	Read date from a named resource. This is shortcut for calling
 *	GetResourceStream()/read from the stream/delete the stream.
 *	This can be convinient when the resource should be read only
 *	once and when the entire resource should be loaded in one
 *	read operation.
 * \note
 *	ReadResource() will always start at offset 0 in the stream so
 *	multiple reads will return the same data.
 * \param cResName
 *	The name of the resource to read.
 * \param pBuffer
 *	Pointer to a buffer that will receive at most \p nSize bytes of data.
 *	If only the mime-type is interresting this can be a NULL pointer.
 * \param pcResType
 *	Pointer to a STL string that will receive the mime-type of the
 *	resource or NULL if you don't want to receive the mime-type.
 * \param nSize
 *	The maximum number of bytes to read from the stream.
 * \return The number of bytes actually read or -1 if an error occure.
 *	   In the case of failure an error code will be written to
 *	   the global variable \b errno.
 * \sa GetResourceStream()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


ssize_t Resources::ReadResource( const String & cResName, void *pBuffer, String * pcResType, ssize_t nSize )
{
	ResStream *pcStream = GetResourceStream( cResName );

	if( pcStream != NULL )
	{
		ssize_t nError = 0;

		if( pBuffer != NULL )
		{
			nError = pcStream->Read( pBuffer, nSize );
		}
		if( pcResType != NULL )
		{
			*pcResType = pcStream->GetType();
		}
		delete pcStream;

		return ( nError );
	}
	else
	{
		return ( -1 );
	}
}

/** Get a seekable-data stream referencing a resource's data.
 * \par Description:
 *	Get a seekable data stream referencing the specified resource.
 *	The returned stream inherit from os::SeekableIO and can be
 *	used to read data from the resource.
 * \note
 *	It is your responsibility to delete the returned stream object
 *	when you are done using it. The returned stream will also
 *	reference it's parent os::Resources object so it is important
 *	that you don't use the stream after the os::Resources object
 *	that spawned it has been deleted.
 * \param cName
 *	The name of the resource to lookup.
 * \return A pointer to a os::ResStream object that can be used to read
 *	   data from the resource and to obtain various other info like
 *	   mime-type and size of the resource. If there was no resource
 *	   with the given name or if something went wrong during
 *	   creation of the stream object NULL will be returned and an
 *	   error code will be written to the global variable \b errno.
 * \sa GetResourceStream(uint), CreateResource(), ReadResource()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

ResStream *Resources::GetResourceStream( const String & cName )
{
	for( uint i = 0; i < m->m_apResources.size(); ++i )
	{
		if( cName == m->m_apResources[i]->m_cName )
		{
			try
			{
				ResStream *pcStream = new ResStream( m->m_pcStream, m->m_apResources[i]->m_nFileOffset + m->m_apResources[i]->m_nHeaderSize,
					cName, m->m_apResources[i]->m_cType, m->m_apResources[i]->m_nSize, true );

				return ( pcStream );
			}
			catch( std::bad_alloc & )
			{
				errno = ENOMEM;
				return ( NULL );
			}
			break;
		}
	}
	errno = ENOENT;
	return ( NULL );
}

/** Get a seekable-data stream referencing a resource's data.
 * \par Description:
 *	GetResourceStream(uint) have the same functionality as
 *	GetResourceStream(const String&) except that the resource is
 *	identified by an index instead of the resource name. This can be
 *	useful when for example iterating over all the resources in an
 *	archive. Look at GetResourceStream(const String&) for a more
 *	detailed description of the functionality.
 * \note
 *	It is your responsibility to delete the returned stream object
 *	when you are done using it. The returned stream will also
 *	reference it's parent os::Resources object so it is important
 *	that you don't use the stream after the os::Resources object
 *	that spawned it has been deleted.
 * \param nIndex
 *	The index (between 0 and GetResourceCount() - 1) of the resource to
 *	obtain.
 * \return Pointer to a os::ResStream object or NULL if the index was out
 *	   of range or if something whent wrong during creation of the
 *	   stream object.
 * \sa GetResourceStream(const String&)
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

ResStream *Resources::GetResourceStream( uint nIndex )
{
	if( nIndex < m->m_apResources.size() )
	{
		try
		{
			return ( new ResStream( m->m_pcStream, m->m_apResources[nIndex]->m_nFileOffset + m->m_apResources[nIndex]->m_nHeaderSize,
				m->m_apResources[nIndex]->m_cName, m->m_apResources[nIndex]->m_cType, m->m_apResources[nIndex]->m_nSize, true ) );
		}
		catch( std::bad_alloc & )
		{
			errno = ENOMEM;
			return ( NULL );
		}
	}
	else
	{
		errno = EINVAL;
		return ( NULL );
	}
}

/** Create a new resource.
 * \par Description:
 *	Create a new resource and return a writable stream object
 *	referencing it. The name, size, and type must be specified
 *	and can not be changed later. Neither can the resource be
 *	deleted again without recreating the archive from scratch.
 * \note
 *	It is your responsibility to delete the returned stream object
 *	when you are done using it. The returned stream will also
 *	reference it's parent os::Resources object so it is important
 *	that you don't use the stream after the os::Resources object
 *	that spawned it has been deleted.
 * \param cName
 *	The name of the new resource. The name must be unique and can be
 *	at most 255 bytes long.
 * \param cType
 *	The mime-type of the stream. This is used to identify the data
 *	type that will be written to the stream. If the data don't have
 *	a mime-typed defined for it you should normally use
 *	"application/octet-stream". The mime-type must be at most 255
 *	bytes long.
 * \param nSize
 *	The number of bytes that should be allocated for this resource.
 * \return Pointer to a writable ResStream object if the resource was
 *	   successfully created or NULL if something went wrong. In the
 *	   case of failure an error code will be written to the global
 *	   variable \b errno.
 * \sa GetResourceStream()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

ResStream *Resources::CreateResource( const String & cName, const String & cType, ssize_t nSize )
{
	ResourceDesc sDesc;

	if( cName.size() > 255 || cType.size(  ) > 255 )
	{
		errno = ENAMETOOLONG;
		return ( NULL );
	}

	sDesc.m_nFileOffset = m->m_nResOffset + m->m_nTotalSize;
	sDesc.m_nHeaderSize = cName.size() + 1 + cType.size(  ) + 1 + sizeof( int );
	sDesc.m_nSize = nSize;
	sDesc.m_cName = cName;
	sDesc.m_cType = cType;


	ssize_t nError;

	nError = m->m_pcStream->Seek( sDesc.m_nFileOffset, SEEK_SET );
	if( nError < 0 )
	{
		return ( NULL );
	}
	nError = m->m_pcStream->Write( &sDesc.m_nSize, sizeof( int ) );
	if( nError != sizeof( int ) )
	{
		return ( NULL );
	}
	uint8 nNameLen = cName.size();

	if( m->m_pcStream->Write( &nNameLen, 1 ) != 1 )
	{
		return ( NULL );
	}
	if( m->m_pcStream->Write( cName.c_str(), nNameLen ) != nNameLen )
	{
		return ( NULL );
	}
	nNameLen = cType.size();
	if( m->m_pcStream->Write( &nNameLen, 1 ) != 1 )
	{
		return ( NULL );
	}
	if( m->m_pcStream->Write( cType.c_str(), nNameLen ) != nNameLen )
	{
		return ( NULL );
	}
	ResStream* pcStream = NULL;
	try
	{
		pcStream = new ResStream( m->m_pcStream, sDesc.m_nFileOffset + sDesc.m_nHeaderSize,
			cName, cType, nSize, false );

		m->m_nTotalSize += nSize + sDesc.m_nHeaderSize;
		ResourceDesc* psDesc = new ResourceDesc( sDesc );
		m->m_apResources.push_back( psDesc );

		m->m_pcStream->WritePos( m->m_nResOffset + sizeof( int ) * 2, &m->m_nTotalSize, sizeof( off_t ) );
		int nValue = m->m_apResources.size();
		m->m_pcStream->WritePos( m->m_nResOffset + sizeof( int ) * 2 + sizeof( off_t ), &nValue, sizeof( int ) );
		return ( pcStream );
	}
	catch( ... )
	{
		if( pcStream != NULL ) delete( pcStream );	/* In case new ResStream succeeded but new ResourceDesc failed */
		errno = ENOMEM;
		return ( NULL );
	}
}

/** Locate the resource section in an AtheOS executable or DLL.
 * \par Description:
 *	Locate the resource section in an AtheOS executable or DLL.
 * \note
 *	This member is normally only used internally by the os::Resources
 *	class itself but it is made public so you can call it if you should
 *	ever need it.
 * \param pcStream
 *	A seekable stream referencing an AtheOS executable or DLL.
 * \param pnOffset
 *	Pointer to an off_t variable that will receive the position of the
 *	first byte of the resource section within the executable. Can safely
 *	be passed as NULL if the offset is not interresting.
 * \param pnSize
 *	Pointer to an ssize_t variable that will receive the size of the
 *	resource section. Can safely be passed as NULL if the size is not
 *	interresting.
 * \param pzSectionName
 *	Should normally be passed as NULL (the default) but can optionally
 *	be used to specify the name of the section to be searched for.
 * \return On success 0 is returned. On failure -1 is returned and an error
 *	   code is written to the global variable \b errno.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Resources::FindExecutableResource( SeekableIO * pcStream, off_t *pnOffset, ssize_t *pnSize, const char *pzSectionName )
{
	Elf32_ElfHdr_s sElfHdr;
	int nError;

	if( pzSectionName == NULL )
	{
		pzSectionName = ".aresources";
	}
	pcStream->Seek( 0LL, SEEK_SET );
	nError = pcStream->Read( &sElfHdr, sizeof( sElfHdr ) );
	if( nError != sizeof( sElfHdr ) )
	{
		if( nError >= 0 )
		{
			errno = ENOEXEC;
		}
		return ( -1 );
	}
	if( sElfHdr.e_nSecHdrSize != sizeof( Elf32_SectionHeader_s ) || sElfHdr.e_nSecNameStrTabIndex >= sElfHdr.e_nSecHdrCount )
	{
		errno = ENOEXEC;
		return ( -1 );
	}
	Elf32_SectionHeader_s sStringSecHdr;

	nError = pcStream->ReadPos( sElfHdr.e_nSecHdrOff + sElfHdr.e_nSecHdrSize * sElfHdr.e_nSecNameStrTabIndex, &sStringSecHdr, sizeof( sStringSecHdr ) );
	if( nError != sizeof( sStringSecHdr ) )
	{
		if( nError >= 0 )
		{
			errno = ENOEXEC;
		}
		return ( -1 );
	}

	Elf32_SectionHeader_s sSectionHdr;

	for( int i = sElfHdr.e_nSecHdrCount - 1; i >= 0; --i )
	{
		nError = pcStream->ReadPos( sElfHdr.e_nSecHdrOff + sElfHdr.e_nSecHdrSize * i, &sSectionHdr, sizeof( sSectionHdr ) );
		if( nError != sizeof( sSectionHdr ) )
		{
			if( nError >= 0 )
			{
				errno = ENOEXEC;
			}
			return ( -1 );
		}
		char zName[16];

		zName[0] = '\0';
		if( pcStream->ReadPos( sStringSecHdr.sh_nOffset + sSectionHdr.sh_nName, zName, sizeof( zName ) ) < 0 )
		{
			return ( -1 );
		}
		if( strcmp( zName, pzSectionName ) == 0 )
		{
			*pnOffset = sSectionHdr.sh_nOffset;
			*pnSize = sSectionHdr.sh_nSize;
			return ( 0 );
		}
	}
	errno = ENOENT;
	return ( -1 );
}
