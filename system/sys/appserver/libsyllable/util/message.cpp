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

#include <malloc.h>
#include <assert.h>
#include <atheos/kernel.h>

#include <util/message.h>
#include <gui/region.h>
#include <util/looper.h>
#include <util/messenger.h>
#include <util/variant.h>

#include <stdexcept>

using namespace os;

#define ARRAY_NAME( array )  reinterpret_cast<char*>(static_cast<DataArray_s*>(psArray)+1)
#define FIRST_CHUNK( array ) reinterpret_cast<Chunk_s*>(ARRAY_NAME((array)) + (psArray)->nNameLength)
#define CHUNK_DATA( chunk )  reinterpret_cast<uint8*>(static_cast<Chunk_s*>(chunk)+1)

/** Constructor
 * \par Description:
 *	Construct an empty message.
 * \param nCode
 *	The message code. This can later be retrieved with GetCode().
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Message::Message( int nCode )
{
	m_pcNext = NULL;
	m_nCode = nCode;
	m_psFirstArray = NULL;
	m_nTargetToken = -1;
	m_nReplyToken = -1;
	m_hReplyPort = -1;
	m_nFlags = 0;
	m_nFlattenedSize = _GetStaticSize();
}

/** Copy constructor
 * \par Description:
 *	Copy the message code and all data members from another message.
 * \param cMsg
 *	The message to copy.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Message::Message( const Message & cMsg )
{
	m_pcNext = NULL;
	m_nCode = 0;
	m_psFirstArray = NULL;
	m_nTargetToken = -1;
	m_nReplyToken = -1;
	m_hReplyPort = -1;
	m_nFlags = 0;
	m_nFlattenedSize = _GetStaticSize();
	*this = cMsg;
}

/** Copy the content of another message.
 * \par Description:
 *	The assignment operator will copy the message code and all data
 *	members from \p cMsg
 * \param cMsg
 *	The message to copy.
 * \return A reference to the message itself.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Message & Message::operator=( const Message & cMsg )
{
	m_nCode = cMsg.m_nCode;
	m_nTargetToken = cMsg.m_nTargetToken;
	m_nReplyToken = cMsg.m_nReplyToken;
	m_hReplyPort = cMsg.m_hReplyPort;
	m_hReplyProc = cMsg.m_hReplyProc;

	m_nFlags = 0;
	m_pcNext = NULL;
	m_psFirstArray = NULL;
	m_nFlattenedSize = cMsg.m_nFlattenedSize;

	DataArray_s *psSrcArray;

	for( psSrcArray = cMsg.m_psFirstArray; psSrcArray != NULL; psSrcArray = psSrcArray->psNext )
	{
		DataArray_s *psNewArray = ( DataArray_s * ) malloc( psSrcArray->nCurSize );

		if( psNewArray != NULL )
		{
			memcpy( psNewArray, psSrcArray, psSrcArray->nCurSize );

			psNewArray->psNext = NULL;
			psNewArray->nMaxSize = psSrcArray->nCurSize;
			_AddArray( psNewArray );
		}
	}
	return ( *this );
}

/** Construct a message from a "flattened" buffer.
 * \par Description:
 *	This constructor can be used to create a "live" message from a
 *	data-blob created by os::Message::Flatten().
 * \note
 *	If the data-blob is not a valid flattened message the
 *	std::invalid_argument exception will be thrown.
 * \param pFlattenedData
 *	Pointer to a data-blob created by a previous call to Flatten().
 * \sa Flatten(), Unflatten()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


Message::Message( const void *pFlattenedData )
{
	m_pcNext = NULL;
	m_nCode = 0;
	m_psFirstArray = NULL;
	m_nTargetToken = -1;
	m_nReplyToken = -1;
	m_hReplyPort = -1;
	m_nFlags = 0;
	m_nFlattenedSize = _GetStaticSize();

	if( Unflatten( ( const uint8 * )pFlattenedData ) )
	{
		throw( std::invalid_argument( "Invalid flatten buffer" ) );
	}
}

/** Destructor.
 * \par Description:
 *	Free all resources allocated by the message.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Message::~Message()
{
	m_nTargetToken = -1;
	MakeEmpty();
}

/** Clear the message.
 * \par Description:
 *	Clear the message and delete all datamembers. The message code
 *	will not be modified.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Message::MakeEmpty()
{
	DataArray_s *psArray;

	while( ( psArray = m_psFirstArray ) )
	{
		m_psFirstArray = psArray->psNext;
		free( psArray );
	}
	m_nFlattenedSize = _GetStaticSize();
}

/** Get the required buffer size needed to "flatten" this message.
 * \par Description:
 *	Call this member to learn how large buffer must be allocated before
 *	calling Flatten().
 * \return The number of bytes needed to flatten this message.
 * \sa Flatten(), Unflatten()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

size_t Message::GetFlattenedSize() const
{
	return ( m_nFlattenedSize );
}

/** Write the message to a flat data-blob.
 * \par Description:
 *	Flatten() will convert the message into a flat data-stream that
 *	lend itself well for transmission over the lowlevel message system
 *	to other processes or for storing a message on disk for later retrival.
 * \par
 *	All the data members, the message code, and any additional data might
 *	needed to reconstruct the exact same message are written to the buffer
 *	in a format understod by the Unflatten() member and the Message::Message(void*)
 *	constructor.
 * \note
 *	To learn how large the \p pBuffer must be you must first call GetFlattenedSize().
 * \param pBuffer
 *	Pointer to a buffer at least as large as reported by GetFlattenedSize()
 *	where the flattened message will be written.
 * \param nSize
 *	Size of the buffer passed as \p pBuffer
 * \return On success 0 is returned. If the buffer is to small -1 is returned
 *	and the global variable \b errno is set to EOVERFLOW.
 * \sa GetFlattenedSize(), Unflatten()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::Flatten( uint8 *pBuffer, size_t nSize ) const
{
	DataArray_s *psArray;
	int nPos = _GetStaticSize();	// Position start after size, and code

	assert( pBuffer != NULL );

	( ( int32 * )pBuffer )[1] = m_nCode;
	( ( int32 * )pBuffer )[2] = m_nTargetToken;
	( ( int32 * )pBuffer )[3] = m_nReplyToken;
	( ( int32 * )pBuffer )[4] = m_hReplyPort;
	( ( int32 * )pBuffer )[5] = m_hReplyProc;
	( ( int32 * )pBuffer )[6] = m_nFlags & ~REPLY_SENT;

	for( psArray = m_psFirstArray; psArray != NULL; psArray = psArray->psNext )
	{
		if( size_t ( nPos + psArray->nCurSize ) > nSize )
		{
			errno = EOVERFLOW;
			return ( -1 );
		}
		memcpy( &pBuffer[nPos], psArray, psArray->nCurSize );
		nPos += psArray->nCurSize;
	}
	( ( size_t * )pBuffer )[0] = nPos;	// Put the size as first int in the buffer
	return ( 0 );
}

/** Reconstruct a message from a data-blob created by Flatten().
 * \par Description:
 *	Unflatten() can reconstruct a message previously flattened with the
 *	Flatten() member.
 * \par
 *	This will reconstruct an excact copy of the message that was used to
 *	create the data-blob.
 * \note
 *	The previous content of the message will be discarded.
 * \param pBuffer
 *	Pointer to a data-blob previously created with Flatten()
 * \return On success 0 is returned. If the data-blob is not a valid flattened
 *	message -1 is returned and the global variable \b errno is et to
 *	EINVAL.
 * \sa Flatten(), Message::Message(void*)
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::Unflatten( const uint8 *pBuffer )
{
	size_t nSize;

	assert( pBuffer != NULL );

	MakeEmpty();

	nSize = ( ( size_t * )pBuffer )[0];
	m_nCode = ( ( int32 * )pBuffer )[1];
	m_nTargetToken = ( ( int32 * )pBuffer )[2];
	m_nReplyToken = ( ( int32 * )pBuffer )[3];
	m_hReplyPort = ( ( int32 * )pBuffer )[4];
	m_hReplyProc = ( ( int32 * )pBuffer )[5];
	m_nFlags = ( ( int32 * )pBuffer )[6] & ~REPLY_SENT;

	size_t nPos = _GetStaticSize();	// Position start after size, and code

	for( ; nPos < nSize; )
	{
		DataArray_s *psSrcArray = ( DataArray_s * ) & pBuffer[nPos];
		DataArray_s *psNewArray = ( DataArray_s * ) malloc( psSrcArray->nCurSize );

		if( psNewArray != NULL )
		{
			memcpy( psNewArray, psSrcArray, psSrcArray->nCurSize );

			psNewArray->psNext = NULL;
			psNewArray->nMaxSize = psSrcArray->nCurSize;
			nPos += psSrcArray->nCurSize;
			_AddArray( psNewArray );
		}
		else
		{
			dbprintf( "Message::Unflatten() out of memory\n" );
			break;
		}
	}
	m_nFlattenedSize = nSize;

	if( nPos != nSize )
	{
		dbprintf( "Message::Unflatten() mismatch between size %d, and bytes read %d\n", nSize, nPos );
		MakeEmpty();
		errno = EINVAL;
		return ( -1 );
	}

	return ( 0 );
}

/** \internal
 *****************************************************************************/

Message::Chunk_s * Message::_GetChunkAddr( DataArray_s * psArray, int nIndex ) const
{
	if( nIndex >= psArray->nCount )
	{
		return ( NULL );
	}
	Chunk_s *psChunk = FIRST_CHUNK( psArray );

	if( psArray->nChunkSize == 0 )
	{
		for( int i = 0; i < nIndex; ++i )
		{
			psChunk = reinterpret_cast < Chunk_s * >( reinterpret_cast < uint8 *>( psChunk ) + psChunk->nDataSize + sizeof( Chunk_s ) );
		}
		return ( psChunk );
	}
	else
	{
		return ( reinterpret_cast < Chunk_s * >( reinterpret_cast < uint8 *>( psChunk ) + psArray->nChunkSize * nIndex ) );
	}
}

/** \internal
 *****************************************************************************/

uint8 *Message::_CreateArray( const char *pzName, int nType, const void *pData, uint32 nSize, bool bFixedSize, int nMaxCountHint )
{
	assert( pzName != NULL );

	DataArray_s *psArray;
	int nNameLen = strlen( pzName );
	int nTotalSize = sizeof( DataArray_s ) + nNameLen + nSize * nMaxCountHint;
	uint8 *pBuffer;

	if( bFixedSize == false )
	{
		nTotalSize += sizeof( Chunk_s ) * nMaxCountHint;
	}

	psArray = ( DataArray_s * ) malloc( nTotalSize );

	if( psArray == NULL )
	{
		return ( NULL );
	}


	psArray->psNext = NULL;
	psArray->nMaxSize = nTotalSize;
	psArray->nCurSize = sizeof( DataArray_s ) + nNameLen + nSize + ( ( bFixedSize ) ? 0 : sizeof( Chunk_s ) );
	psArray->nCount = 1;
	psArray->nTypeCode = nType;
	psArray->nNameLength = nNameLen;

	m_nFlattenedSize += psArray->nCurSize;

	memcpy( ARRAY_NAME( psArray ), pzName, nNameLen );

	Chunk_s *psChunk = FIRST_CHUNK( psArray );	// (Chunk_s*) (((uint8*)psArray) + ArrayHdrSize() + nNameLen);

	if( bFixedSize )
	{
		psArray->nChunkSize = nSize;
		pBuffer = ( uint8 * )psChunk;
	}
	else
	{
		psChunk->nDataSize = nSize;
		psArray->nChunkSize = 0;
		pBuffer = CHUNK_DATA( psChunk );
	}
	if( pData != NULL )
	{
		memcpy( pBuffer, pData, nSize );
	}
	_AddArray( psArray );
	return ( pBuffer );
}

/** \internal
 *****************************************************************************/

uint8 *Message::_ExpandArray( DataArray_s * psArray, const void *pData, uint32 nSize )
{
	int nNewSize = psArray->nCurSize + ( ( psArray->nChunkSize == 0 ) ? nSize + sizeof( Chunk_s ) : nSize );

	if( nNewSize > psArray->nMaxSize )
	{
		DataArray_s *psNewArray = ( DataArray_s * ) malloc( nNewSize );

		if( psNewArray != NULL )
		{
			_RemoveArray( psArray );
			memcpy( psNewArray, psArray, psArray->nCurSize );
			free( psArray );
			psArray = psNewArray;

			psArray->psNext = NULL;
			psArray->nMaxSize = nNewSize;
			_AddArray( psArray );
		}
		else
		{
			return ( NULL );
		}
	}

	Chunk_s *psChunk = ( Chunk_s * ) ( ( ( uint8 * )psArray ) + psArray->nCurSize );
	uint8 *pBuffer;

	if( psArray->nChunkSize != 0 )
	{
		pBuffer = ( uint8 * )psChunk;
		psArray->nCurSize += nSize;
		m_nFlattenedSize += nSize;
	}
	else
	{
		psChunk->nDataSize = nSize;
		pBuffer = CHUNK_DATA( psChunk );	//->aData;
		psArray->nCurSize += nSize + sizeof( Chunk_s );
		m_nFlattenedSize += nSize + sizeof( Chunk_s );
	}
	psArray->nCount++;

	if( pData != NULL )
	{
		memcpy( pBuffer, pData, nSize );
	}
	return ( pBuffer );
}

/** \internal
 *****************************************************************************/

Message::DataArray_s * Message::_FindArray( const char *pzName, int nType ) const
{
	DataArray_s *psArray;

	assert( pzName != NULL );
	int nNameLen = strlen( pzName );

	for( psArray = m_psFirstArray; psArray != NULL; psArray = psArray->psNext )
	{
		if( ( nType == T_ANY_TYPE || nType == psArray->nTypeCode ) && nNameLen == psArray->nNameLength )
		{
			if( strncmp( pzName, ARRAY_NAME( psArray ), nNameLen ) == 0 )
			{
				return ( psArray );
			}
		}
	}
	return ( NULL );
}

/** \internal
 *****************************************************************************/

void Message::_AddArray( DataArray_s * psArray )
{
	psArray->psNext = m_psFirstArray;
	m_psFirstArray = psArray;
}

/** \internal
 *****************************************************************************/

void Message::_RemoveArray( DataArray_s * psRemArray )
{
	DataArray_s *psArray;

	if( m_psFirstArray == psRemArray )
	{
		m_psFirstArray = psRemArray->psNext;
	}
	else
	{
		for( psArray = m_psFirstArray; psArray != NULL; psArray = psArray->psNext )
		{
			if( psArray->psNext == psRemArray )
			{
				psArray->psNext = psRemArray->psNext;
			}
		}
	}
}

/** Add a data member to the message.
 * \par Description:
 *	Add data to the message. The os::Message class is a very flexible data
 *	container. Each data-member is associated with a name for later retrival.
 *	Multiple data elements of the same type can be arranged as an array
 *	associated with a single name.
 * \par
 *	AddData() is a "catch all" function that can be used to add an
 *	untyped blob of data to the message. Normally you will use one
 *	of the other Add*() member that add a specificly typed data member
 *	but I will describe the general principle of adding member to a
 *	message here.
 * \par
 *	When you add a member to a message you must give it a unique name
 *	that will later be used to lookup that member. It is possible to
 *	add multiple elements with the same name but they must then be
 *	of the same type and subsequent elements will be appended to an
 *	array associated with this name. Individual elements from the
 *	array can later be destined with the name/index pair passed to
 *	the various Get*() members.
 * \note
 *	The data buffer are copyed into the message and you retain ownership
 *	over it.
 * \param pzName
 *	The name used to identify the member. If there already exist a member
 *	with this name in the message the new member will be appended to
 *	an array of elements under this name. It is an error to append
 *	objects with different types under the same name.
 * \param nType
 *	Data type. This should be one of the predefined
 *	\ref os_util_typecodes "type codes".
 *	Normally you should only use this member to add T_RAW type data.
 *	All the more specific data-types should be added with one of
 *	the specialized Add*() members.
 * \param pData
 *	Pointer to the data to be add. The data will be copyed into the message.
 * \param nSize
 *	Size of the \p pData buffer.
 * \param bFixedSize
 *	If you plan to make an array of members under the same name you must
 *	let the message know if each element can have a different size. If
 *	the message know that all the elements have the same size it can
 *	optimize the data a bit by only storing the size once. It will also
 *	greatly speed up array-element lookups if each element have the
 *	same size. If all elements in an array will have the same size or
 *	if you plan to add only one member under this name \p bFixedSize
 *	should be \b true. Otherwhice it should be \b false.
 * \param nMaxCountHint
 *	An estimate of how many members are going to be added to this array.
 *	If you plan to add many elements under the same name and you know
 *	up-front how many you are going to add it is a good idea to let
 *	the message know when adding the first element. The \p nMaxCountHint
 *	will be used to decide how much memory to be allocated for the array
 *	and if the estimate is correct it will avoid any expensive reallocations
 *	during element insertions.
 * \return On success 0 is returned. On error -1 is returned an an error code
 *	is written to the global variable \b errno.
 * \sa FindData(), RemoveData(), RemoveName(), GetNameInfo()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddData( const char *pzName, int nType, const void *pData, uint32 nSize, bool bFixedSize, int nMaxCountHint )
{
	assert( pzName != NULL );
	assert( pData != NULL );

	DataArray_s *psArray = _FindArray( pzName, nType );

	if( psArray != NULL )
	{
		if( psArray->nChunkSize == 0 || uint32 ( psArray->nChunkSize ) == nSize )
		{
			// NOTE: The array may be moved in memory during expansion
			if( _ExpandArray( psArray, pData, nSize ) != NULL )
			{
				return ( 0 );
			}
		}
	}
	else
	{
		if( _CreateArray( pzName, nType, pData, nSize, bFixedSize, nMaxCountHint ) != NULL )
		{
			return ( 0 );
		}
	}
	return ( -1 );
}

/** Delete an element from the message.
 * \par Description:
 *	Delete a single element from a message member.
 * \param pzName
 *	The member name.
 * \param nIndex
 *	Index of the element to delete. If more than one element was added
 *	under the same name you can select which one to delete with this
 *	index.
 * \return On success 0 is returned. If the name was not found or the index
 *	was out of range -1 is returned and a error code is written to the
 *	global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOENT	The name was not found.
 *	<dd>EINVAL	The index was out of range.
 *	</dl>
 * \sa RemoveName(),AddData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::RemoveData( const char *pzName, int nIndex )
{
	DataArray_s *psArray = _FindArray( pzName, T_ANY_TYPE );

	if( psArray == NULL )
	{
		errno = ENOENT;
		return ( -1 );
	}

	if( nIndex == 0 && psArray->nCount == 1 )
	{
		_RemoveArray( psArray );
		free( psArray );
		return ( 0 );
	}

	Chunk_s *psChunk = _GetChunkAddr( psArray, nIndex );

	if( psChunk == NULL )
	{
		errno = EINVAL;
		return ( -1 );
	}

	int nSize;

	if( psArray->nChunkSize == 0 )
	{
		nSize = psChunk->nDataSize + sizeof( Chunk_s );
	}
	else
	{
		nSize = psArray->nChunkSize;
	}

	if( nIndex != psArray->nCount - 1 )
	{
		uint8 *pFirstChunk = reinterpret_cast < uint8 *>( FIRST_CHUNK( psArray ) );
		uint8 *pData = reinterpret_cast < uint8 *>( psChunk );
		int nCurSize = psArray->nCurSize - ( sizeof( DataArray_s ) + psArray->nNameLength );

		memmove( pData, pData + nSize, nCurSize - ( pData - pFirstChunk ) - nSize );
	}

	psArray->nCount--;
	psArray->nCurSize -= nSize;
	m_nFlattenedSize -= nSize;
	return ( 0 );
}

/** Remove all entries stored under a given name.
 * \par Description:
 *	Delete all entries stored under a given name.
 * \param pzName
 *	The name of the member to delete.
 * \return On success 0 is returned. If the name was not found -1 is returned
 *	and a error code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOENT	The name was not found.
 *	</dl>
 * \sa RemoveData(), AddData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::RemoveName( const char *pzName )
{
	DataArray_s *psArray = _FindArray( pzName, T_ANY_TYPE );

	if( psArray == NULL )
	{
		errno = ENOENT;
		return ( -1 );
	}
	m_nFlattenedSize -= psArray->nCurSize;
	_RemoveArray( psArray );
	free( psArray );
	return ( 0 );
}

/** Add an object as a member of this message.
 * \par Description:
 *	Add a flattenable object as a member of this message.
 * \par
 *	See AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param pcVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindObject(), AddData(), FindData()
 * \author Henrik Isaksson
 *****************************************************************************/

status_t Message::AddObject( const char *pzName, const Flattenable& cVal )
{
	DataArray_s *psArray;

	assert( pzName != NULL );

	size_t nSize = cVal.GetFlattenedSize();

	psArray = _FindArray( pzName, cVal.GetType() );

	uint8 *pBuffer;

	if( psArray == NULL )
	{
		pBuffer = _CreateArray( pzName, cVal.GetType(), NULL, nSize, false, 1 );
	}
	else
	{
		pBuffer = _ExpandArray( psArray, NULL, nSize );
	}
	if( pBuffer != NULL )
	{
		cVal.Flatten( pBuffer, nSize );
		return ( 0 );
	}
	else
	{
		errno = ENOMEM;
		return ( -1 );
	}
}

/** Add another message object as a member of this message.
 * \par Description:
 *	Add another message object as a member of this message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param pcVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindMessage(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddMessage( const char *pzName, const Message * pcVal )
{
	DataArray_s *psArray;

	assert( pzName != NULL );
	assert( pcVal != NULL );

	size_t nSize = pcVal->GetFlattenedSize();

	psArray = _FindArray( pzName, T_MESSAGE );

	uint8 *pBuffer;

	if( psArray == NULL )
	{
		pBuffer = _CreateArray( pzName, T_MESSAGE, NULL, nSize, false, 1 );
	}
	else
	{
		pBuffer = _ExpandArray( psArray, NULL, nSize );
	}
	if( pBuffer != NULL )
	{
		pcVal->Flatten( pBuffer, nSize );
		return ( 0 );
	}
	else
	{
		errno = ENOMEM;
		return ( -1 );
	}
}

/** Add a pointer to the message.
 * \par Description:
 *	Add a pointer to the message. Only the pointer itself will be stored
 *	not the data it is pointing to. This means that you can not send
 *	the message to another process and access the pointer there. You
 *	must also be careful not to delete the object the pointer referre
 *	to before the receiver of the message is done using it.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param pVal
 *	The pointer you want to add. Only the value of the pointer will
 *	be stored. Not the object it is pointing at.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindPointer(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddPointer( const char *pzName, const void *pVal )
{
	return ( AddData( pzName, T_POINTER, &pVal, sizeof( pVal ) ) );
}

/** Add a 8-bit integer to the message.
 * \par Description:
 *	Add a 8-bit integer to the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param nVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindInt8(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddInt8( const char *pzName, int8 nVal )
{
	return ( AddData( pzName, T_INT8, &nVal, sizeof( nVal ) ) );
}

/** Add a 16-bit integer to the message.
 * \par Description:
 *	Add a 16-bit integer to the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param nVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindInt16(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddInt16( const char *pzName, int16 nVal )
{
	return ( AddData( pzName, T_INT16, &nVal, sizeof( nVal ) ) );
}

/** Add a 32-bit integer to the message.
 * \par Description:
 *	Add a 32-bit integer to the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param nVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindInt32(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddInt32( const char *pzName, int32 nVal )
{
	return ( AddData( pzName, T_INT32, &nVal, sizeof( nVal ) ) );
}

/** Add a 64-bit integer to the message.
 * \par Description:
 *	Add a 64-bit integer to the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param nVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindInt64(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddInt64( const char *pzName, int64 nVal )
{
	return ( AddData( pzName, T_INT64, &nVal, sizeof( nVal ) ) );
}

/** Add a boolean value to the message.
 * \par Description:
 *	Add a boolean value to the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param bVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindBool(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddBool( const char *pzName, bool bVal )
{
	return ( AddData( pzName, T_BOOL, &bVal, sizeof( bVal ) ) );
}

/** Add a float to the message.
 * \par Description:
 *	Add a float to the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param vVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindFloat(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddFloat( const char *pzName, float vVal )
{
	return ( AddData( pzName, T_FLOAT, &vVal, sizeof( vVal ) ) );
}

/** Add a double to the message.
 * \par Description:
 *	Add a double to the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param vVal
 *	The object to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindDouble(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddDouble( const char *pzName, double vVal )
{
	return ( AddData( pzName, T_DOUBLE, &vVal, sizeof( vVal ) ) );
}

/** Add a string to the message.
 * \par Description:
 *	Add a string to the message. The string must be NUL terminated
 *	and will be copyed into the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \note
 *	The two string types (old fashion C-string and STL strings) are
 *	fully interchangable. They are stored the same way internally
 *	so it does not matter which member it was added with or what
 *	member it is retrieved with. You can add it as an C-string and
 *	look it up again as an STL string.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param pzString
 *	The string to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindString(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddString( const char *pzName, const char *pzString )
{
	return ( AddData( pzName, T_STRING, pzString, strlen( pzString ) + 1, false ) );
}

/** Add a string to the message.
 * \par Description:
 *	Add a STL string to the message.
 * \par
 *	Set AddData() for more details on how to add members to the message.
 * \note
 *	The two string types (old fashion C-string and STL strings) are
 *	fully interchangable. They are stored the same way internally
 *	so it does not matter which member it was added with or what
 *	member it is retrieved with. You can add it as an C-string and
 *	look it up again as an STL string.
 * \param pzName
 *	A name uniquely identifying the added object within the message.
 *	It is possible to add multiple objects of the same type under
 *	the same name but it is an error to use the same name for two
 *	objects of different type.
 * \param cString
 *	The string to add.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno.
 * \par Error codes:
 *	<dl>
 *	<dd>ENOMEM	Not enough memory to expand the message.
 *	</dl>
 * \sa FindString(), AddData(), FindData()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Message::AddString( const char *pzName, const std::string & cString )
{
	return ( AddData( pzName, T_STRING, cString.c_str(), cString.size(  ) + 1, false ) );
}

status_t Message::AddString( const char *pzName, const String & cString )
{
	return ( AddData( pzName, T_STRING, cString.c_str(), cString.size(  ) + 1, false ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::AddRect( const char *pzName, const Rect & cVal )
{
	return ( AddData( pzName, T_RECT, &cVal, sizeof( cVal ) ) );
}

status_t Message::AddIRect( const char *pzName, const IRect & cVal )
{
	return ( AddData( pzName, T_IRECT, &cVal, sizeof( cVal ) ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::AddPoint( const char *pzName, const Point & cVal )
{
	return ( AddData( pzName, T_POINT, &cVal, sizeof( cVal ) ) );
}

status_t Message::AddIPoint( const char *pzName, const IPoint & cVal )
{
	return ( AddData( pzName, T_IPOINT, &cVal, sizeof( cVal ) ) );
}

status_t Message::AddColor32( const char *pzName, const Color32_s & cVal )
{
	return ( AddData( pzName, T_COLOR32, &cVal, sizeof( cVal ) ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::AddVariant( const char *pzName, const Variant & cVal )
{
	DataArray_s *psArray;

	assert( pzName != NULL );

	size_t nSize = cVal.GetFlattenedSize();

	psArray = _FindArray( pzName, T_VARIANT );

	uint8 *pBuffer;

	if( psArray == NULL )
	{
		pBuffer = _CreateArray( pzName, T_VARIANT, NULL, nSize, false, 1 );
	}
	else
	{
		pBuffer = _ExpandArray( psArray, NULL, nSize );
	}
	if( pBuffer != NULL )
	{
		cVal.Flatten( pBuffer, nSize );
		return ( 0 );
	}
	else
	{
		errno = ENOMEM;
		return ( -1 );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindData( const char *pzName, int nType, const void **ppData, size_t *pnSize, int nIndex ) const
{
	DataArray_s *psArray = _FindArray( pzName, nType );

	if( psArray != NULL )
	{
		Chunk_s *psChunk = _GetChunkAddr( psArray, nIndex );

		if( psChunk != NULL )	// Calculate address of the actual data chunk based on the index
		{
			if( psArray->nChunkSize == 0 )
			{
				if( ppData != NULL )
				{
					*ppData = ( void * )CHUNK_DATA( psChunk );
				}
				if( pnSize != NULL )
				{
					*pnSize = psChunk->nDataSize;
				}
			}
			else
			{
				if( ppData != NULL )
				{
					*ppData = ( void * )psChunk;
				}
				if( pnSize != NULL )
				{
					*pnSize = psArray->nChunkSize;
				}
			}
			return ( 0 );
		}
	}
	return ( -1 );
}

status_t Message::FindObject( const char *pzName, Flattenable & cVal, int nIndex ) const
{
	const void *pBuffer;
	size_t nSize;
	status_t nError = FindData( pzName, cVal.GetType(), &pBuffer, &nSize, nIndex );

	if( nError < 0 )
	{
		return ( nError );
	}
	return ( cVal.Unflatten( ( uint8 * )pBuffer ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindMessage( const char *pzName, Message * pcVal, int nIndex ) const
{
	const void *pBuffer;
	size_t nSize;
	status_t nError = FindData( pzName, T_MESSAGE, &pBuffer, &nSize, nIndex );

	if( nError < 0 )
	{
		return ( nError );
	}
	return ( pcVal->Unflatten( ( uint8 * )pBuffer ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindPointer( const char *pzName, void **ppVal, int nIndex ) const
{
	assert( ppVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_POINTER );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			void **ppData = ( void ** )FIRST_CHUNK( psArray );

			*ppVal = ppData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindInt8( const char *pzName, int8 *pnVal, int nIndex ) const
{
	assert( pnVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_INT8 );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			int8 *pnData = reinterpret_cast < int8 *>( FIRST_CHUNK( psArray ) );

			*pnVal = pnData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindInt16( const char *pzName, int16 *pnVal, int nIndex ) const
{
	assert( pnVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_INT16 );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			int16 *pnData = reinterpret_cast < int16 *>( FIRST_CHUNK( psArray ) );

			*pnVal = pnData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindInt32( const char *pzName, int32 *pnVal, int nIndex ) const
{
	assert( pnVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_INT32 );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			int32 *pnData = reinterpret_cast < int32 *>( FIRST_CHUNK( psArray ) );

			*pnVal = pnData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

status_t Message::FindInt64( const char *pzName, int64 *pnVal, int nIndex ) const
{
	assert( pnVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_INT64 );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			int64 *pnData = reinterpret_cast < int64 *>( FIRST_CHUNK( psArray ) );

			*pnVal = pnData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindBool( const char *pzName, bool *pbVal, int nIndex ) const
{
	assert( pbVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_BOOL );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			bool *pbData = reinterpret_cast < bool *>( FIRST_CHUNK( psArray ) );

			*pbVal = pbData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindFloat( const char *pzName, float *pvVal, int nIndex ) const
{
	assert( pvVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_FLOAT );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			float *pvData = reinterpret_cast < float *>( FIRST_CHUNK( psArray ) );

			*pvVal = pvData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindDouble( const char *pzName, double *pvVal, int nIndex ) const
{
	assert( pvVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_DOUBLE );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			double *pvData = reinterpret_cast < double *>( FIRST_CHUNK( psArray ) );

			*pvVal = pvData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindRect( const char *pzName, Rect * pcVal, int nIndex ) const
{
	assert( pcVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_ANY_TYPE );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			if( psArray->nTypeCode == T_RECT )
			{
				Rect *pcData = reinterpret_cast < Rect * >( FIRST_CHUNK( psArray ) );

				*pcVal = pcData[nIndex];
				return ( 0 );
			}
			else if( psArray->nTypeCode == T_IRECT )
			{
				IRect *pcData = reinterpret_cast < IRect * >( FIRST_CHUNK( psArray ) );

				*pcVal = static_cast < Rect > ( pcData[nIndex] );
				return ( 0 );
			}
		}
	}
	return ( -1 );
}

status_t Message::FindIRect( const char *pzName, IRect * pcVal, int nIndex ) const
{
	assert( pcVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_ANY_TYPE );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			if( psArray->nTypeCode == T_IRECT )
			{
				IRect *pcData = reinterpret_cast < IRect * >( FIRST_CHUNK( psArray ) );

				*pcVal = pcData[nIndex];
				return ( 0 );
			}
			else if( psArray->nTypeCode == T_RECT )
			{
				Rect *pcData = reinterpret_cast < Rect * >( FIRST_CHUNK( psArray ) );

				*pcVal = static_cast < IRect > ( pcData[nIndex] );
				return ( 0 );
			}
		}
	}
	return ( -1 );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindPoint( const char *pzName, Point * pcVal, int nIndex ) const
{
	assert( pcVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_ANY_TYPE );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			if( psArray->nTypeCode == T_POINT )
			{
				Point *pcData = reinterpret_cast < Point * >( FIRST_CHUNK( psArray ) );

				*pcVal = pcData[nIndex];
				return ( 0 );
			}
			else if( psArray->nTypeCode == T_IPOINT )
			{
				IPoint *pcData = reinterpret_cast < IPoint * >( FIRST_CHUNK( psArray ) );

				*pcVal = static_cast < Point > ( pcData[nIndex] );
				return ( 0 );
			}
		}
	}
	return ( -1 );
}

status_t Message::FindIPoint( const char *pzName, IPoint * pcVal, int nIndex ) const
{
	assert( pcVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_ANY_TYPE );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			if( psArray->nTypeCode == T_IPOINT )
			{
				IPoint *pcData = reinterpret_cast < IPoint * >( FIRST_CHUNK( psArray ) );

				*pcVal = pcData[nIndex];
				return ( 0 );
			}
			else if( psArray->nTypeCode == T_POINT )
			{
				Point *pcData = reinterpret_cast < Point * >( FIRST_CHUNK( psArray ) );

				*pcVal = static_cast < IPoint > ( pcData[nIndex] );
				return ( 0 );
			}
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindColor32( const char *pzName, Color32_s * pcVal, int nIndex ) const
{
	assert( pcVal != NULL );

	DataArray_s *psArray = _FindArray( pzName, T_COLOR32 );

	if( psArray != NULL )
	{
		if( nIndex < psArray->nCount )
		{
			Color32_s *pcData = reinterpret_cast < Color32_s * >( FIRST_CHUNK( psArray ) );

			*pcVal = pcData[nIndex];
			return ( 0 );
		}
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindString( const char *pzName, const char **ppzString, int nIndex ) const
{
	if( FindData( pzName, T_STRING, ( const void ** )ppzString, NULL, nIndex ) == 0 )
	{
		return ( 0 );
	}
	return ( -1 );
}

status_t Message::FindString( const char *pzName, std::string * pcString, int nIndex ) const
{
	const char *pzString;

	if( FindData( pzName, T_STRING, ( const void ** )&pzString, NULL, nIndex ) == 0 )
	{
		*pcString = pzString;
		return ( 0 );
	}
	return ( -1 );
}

status_t Message::FindString( const char *pzName, String * pcString, int nIndex ) const
{
	const char *pzString;

	if( FindData( pzName, T_STRING, ( const void ** )&pzString, NULL, nIndex ) == 0 )
	{
		*pcString = pzString;
		return ( 0 );
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::FindVariant( const char *pzName, Variant * pcVal, int nIndex ) const
{
	const void *pBuffer;
	size_t nSize;
	status_t nError = FindData( pzName, T_VARIANT, &pBuffer, &nSize, nIndex );

	if( nError < 0 )
	{
		return ( nError );
	}
	return ( pcVal->Unflatten( ( uint8 * )pBuffer, nSize ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::GetNameInfo( const char *pzName, int *pnType, int *pnCount ) const
{
	DataArray_s *psArray = _FindArray( pzName, T_ANY_TYPE );

	if( psArray != NULL )
	{
		if( pnType != NULL )
		{
			*pnType = psArray->nTypeCode;
		}
		if( pnCount != NULL )
		{
			*pnCount = psArray->nCount;
		}
		return ( 0 );
	}
	return ( -1 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int Message::CountNames( void ) const
{
	DataArray_s *psArray;
	int nCount = 0;

	for( psArray = m_psFirstArray; psArray != NULL; psArray = psArray->psNext )
	{
		nCount++;
	}
	return ( nCount );
}

String Message::GetName( int nIndex ) const
{
	DataArray_s *psArray;

	for( psArray = m_psFirstArray; psArray != NULL; psArray = psArray->psNext )
	{
		if( nIndex-- == 0 )
		{
			return ( String( ARRAY_NAME( psArray ), psArray->nNameLength ) );
		}
	}
	return ( String( "" ) );
}


//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Message::IsEmpty( void ) const
{
	return ( m_psFirstArray == NULL );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Message::WasDelivered( void ) const
{
	return ( m_nFlags & WAS_DELIVERED );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Message::IsSourceWaiting( void ) const
{
	return ( WasDelivered() && ( m_nFlags & REPLY_REQUIRED ) && ( m_nFlags & REPLY_SENT ) == 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Message::IsSourceRemote( void ) const
{
	return ( get_process_id( NULL ) != m_hReplyProc );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

Messenger Message::ReturnAddress( void ) const
{
	return ( Messenger( m_hReplyPort, m_nReplyToken, m_hReplyProc ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool Message::IsReply( void ) const
{
	return ( m_nFlags & IS_REPLY );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::SendReply( int nCode, Handler * pcReplyHandler )
{
	Message cMsg( nCode );

	return ( SendReply( &cMsg, pcReplyHandler ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::SendReply( Message * pcTheReply, Handler * pcReplyHandler, uint nTimeOut )
{
	if( IsSourceWaiting() == false )
	{
		dbprintf( "Message::SendReply() source is not waiting\n" );
		errno = EINVAL;
		return ( -1 );
	}
	if( m_hReplyPort < 0 )
	{
		dbprintf( "Error: Message::SendReply() source is waiting but reply-port is invalid: %d\n", m_hReplyPort );
		errno = EINVAL;
		return ( -1 );
	}

	m_nFlags |= REPLY_SENT;

	Looper *pcLooper = NULL;
	port_id hReplyPort = -1;
	port_id hReplyProc = -1;
	int nReplyToken = -1;

	if( NULL != pcReplyHandler )
	{
		pcLooper = pcReplyHandler->GetLooper();
		if( NULL != pcLooper )
		{
			hReplyPort = pcLooper->GetMsgPort();
			hReplyProc = get_thread_proc( pcLooper->GetThread() );
			nReplyToken = pcReplyHandler->m_nToken;
		}
	}

	uint32 nOldFlags = pcTheReply->m_nFlags;

	pcTheReply->m_nFlags |= IS_REPLY;
	status_t nError = pcTheReply->_Post( m_hReplyPort, m_nReplyToken, hReplyPort, nReplyToken, hReplyProc );

	pcTheReply->m_nFlags = nOldFlags;

	if( nError < 0 )
	{
		dbprintf( "Error: Message::SendReply:1() failed to send message\n" );
	}
	return ( nError );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::SendReply( int nCode, Message * pcReplyToReply )
{
	Message cMsg( nCode );

	return ( SendReply( &cMsg, pcReplyToReply ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::SendReply( Message * pcTheReply, Message * pcReplyToReply, int nSendTimOut, int nReplyTimeOut )
{
	if( IsSourceWaiting() == false )
	{
		dbprintf( "Message::SendReply() source is not waiting\n" );
		errno = EINVAL;
		return ( -1 );
	}
	if( m_hReplyPort < 0 )
	{
		dbprintf( "Error: Message::SendReply() source is waiting but reply-port is invalid: %d\n", m_hReplyPort );
		errno = EINVAL;
		return ( -1 );
	}

	m_nFlags |= REPLY_SENT;

	port_id hReplyPort = -1;

	if( NULL != pcReplyToReply )
	{
		hReplyPort = create_port( "tmp_reply_port", DEFAULT_PORT_SIZE );
		if( hReplyPort < 0 )
		{
			dbprintf( "Error: Message::SendReply() failed to create temporary message port\n" );
			return ( -1 );
		}
	}

	status_t nError = pcTheReply->_Post( m_hReplyPort, m_nReplyToken, hReplyPort, -1, get_process_id( NULL ) );

	if( nError == 0 )
	{
		if( NULL != pcReplyToReply )
		{
			nError = pcReplyToReply->_ReadPort( hReplyPort );
		}
	}
	else
	{
		dbprintf( "Error: Message::SendReply:2() failed to send message\n" );
	}
	if( -1 != hReplyPort )
	{
		delete_port( hReplyPort );
	}
	return ( nError );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::_Post( port_id hPort, uint32 nTarget, port_id hReplyPort, int nReplyTarget, proc_id hReplyProc )
{
	uint8 *pBuffer;

	m_nTargetToken = nTarget;
	m_nReplyToken = nReplyTarget;
	m_hReplyPort = hReplyPort;
	m_hReplyProc = hReplyProc;
	

	// Should'nt this be delayed til the message is received???????????????????????

	uint32 nOldFlags = m_nFlags;

	m_nFlags |= WAS_DELIVERED;

	if( hReplyPort != -1 )
	{
		m_nFlags |= REPLY_REQUIRED;
	}

	int nSize = GetFlattenedSize();

	int nError = 0;

	try
	{
		pBuffer = new uint8[nSize];

		nError = Flatten( pBuffer, nSize );
		if( nError == 0 )
		{
			nError = send_msg( hPort, m_nCode, pBuffer, nSize );
			if( nError < 0 )
			{
				dbprintf( "Error: Message::_Post() failed to send message to %d\n", hPort );
			}
		}
		else
		{
			dbprintf( "Error: Message failed to flatten itself\n" );
		}
		delete[]pBuffer;
	}
	catch( ... )
	{
		dbprintf( "Error: Message::_Post() failed to alloc memory for flatten buffer\n" );
		nError = -1;
		errno = ENOMEM;
	}
	m_nFlags = nOldFlags;
	return ( nError );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void Message::_SetReplyHandler( Handler * pcHandler )
{
	Looper *pcLooper = NULL;

	m_hReplyPort = -1;
	m_nReplyToken = -1;
	m_hReplyProc = -1;

	if( NULL != pcHandler )
	{
		pcLooper = pcHandler->GetLooper();
		if( NULL != pcLooper )
		{
			m_hReplyPort = pcLooper->GetMsgPort();
			m_nReplyToken = pcHandler->m_nToken;
			m_hReplyProc = get_process_id( NULL );
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

status_t Message::_ReadPort( port_id hPort, bigtime_t nTimeOut )
{
	uint8 *pBuffer = new uint8[8192];	// FIXME: Check size first
	int nError = -1;

	if( get_msg_x( hPort, NULL, pBuffer, 8192, nTimeOut ) >= 0 )
	{
		nError = Unflatten( pBuffer );
	}
	delete[]pBuffer;

	return ( nError );
}



