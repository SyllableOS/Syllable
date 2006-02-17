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

#ifndef	__F_UTIL_MESSAGE_H__
#define	__F_UTIL_MESSAGE_H__

#include <sys/types.h>
#include <atheos/types.h>
#include <util/typecodes.h>

#include <util/string.h>

#include "flattenable.h"

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent

class Rect;
class IRect;
class Point;
class IPoint;
class Looper;
class Handler;
class Messenger;
class Variant;
class Settings;
struct Color32_s;


/** 
 * \ingroup util
 * \par Description:
 *	The os::Message is the heart of the messaging system in AtheOS.
 *	An os::Message is a flexible associative container with a few
 *	extra features that makes it suitable for storing messages that
 *	should be sendt over a low-level messaging system like the AtheOS
 *	message-port API or any other package or stream based communication
 *	medium like pipes, TCP, UDP, etc etc.
 * \par
 *	The message associate each message element with a unique name that
 *	is assigned by the sender and that will be used by the receiver to
 *	lookup the element and retrieve it's data. Since each elements are
 *	recognized by a name instead of it's position within a regular
 *	structure it is easy to keep a message protocol forward and backward
 *	compatible. Often it will be possible for a receiver to come up with
 *	reasonable default values for elements that are missing because the
 *	sender is to old and it will simply ignore elements it doesn't know
 *	about because the sender is to new.
 * \par
 *	A message element can either be a single object (an int,
 *	float, a rectangel, a undefined data-blob, etc, etc) or
 *	an array of objects of the same type. Normally each object
 *	added to a message have unique names but it is also possible
 *	to add multiple objects under the same name. Each object must
 *	then be of the same type and they will be added to an array
 *	associated with the name. Individual elements from the array
 *	can then be retrieved by specifying an index in addition to
 *	the name when looking up an entry in the message.
 * \par
 *	What distinguish the os::Message most from other associative
 *	containers like STL maps beside the possibility to add multiple
 *	data-types to a single container is it's ability to convert
 *	itself to a flat data-buffer that lend itself for easy
 *	transmission over most low-level communication channels.
 *	Another message object can then be initialized from this
 *	data-buffer to create an excact copy of the message.
 * \sa os::Messenger, os::Looper, os::Handler, os::Invoker
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class	Message
{
public:
    Message( int nCode = 0 );
    Message( const Message& cMsg );
    Message( const void* pFlattenedData );
    ~Message();

    int		GetCode( void )	const {	return( m_nCode );	}
    void	SetCode( int nCode )  { m_nCode	= nCode;	}

    size_t	GetFlattenedSize( void ) const;
    status_t	Flatten( uint8* pBuffer, size_t nSize ) const;
    status_t	Unflatten( const uint8* pBuffer );

    status_t	AddData( const char* pzName, int nType, const void* pData,
			 uint32 nSize, bool bFixedSize = true, int nMaxCountHint = 1 );

    status_t	AddObject( const char* pzName, const Flattenable& cVal );

    status_t	AddMessage( const char* pzName, const Message* pcVal );
    status_t	AddPointer( const char* pzName, const void* pVal );
    status_t	AddInt8( const char* pzName, int8 nVal );
    status_t	AddInt16( const char* pzName, int16 nVal );
    status_t	AddInt32( const char* pzName, int32 nVal );
    status_t	AddInt64( const char* pzName, int64 nVal );
    status_t	AddBool( const char* pzName, bool bVal );
    status_t	AddFloat( const char* pzName, float vVal );
    status_t	AddDouble( const char* pzName, double vVal );
    status_t	AddRect( const char* pzName, const Rect& cVal );
    status_t	AddIRect( const char* pzName, const IRect& cVal );
    status_t	AddPoint( const char* pzName, const Point& cVal );
    status_t	AddIPoint( const char* pzName, const IPoint& cVal );
    status_t	AddColor32( const char* pzName, const Color32_s& cVal );
    status_t	AddString( const char* pzName, const char* pzString );
    status_t	AddString( const char* pzName, const std::string& cString );
    status_t	AddString( const char* pzName, const String& cString );
    status_t	AddVariant( const char* pzName, const Variant& cVal );

    status_t	FindData( const char* pzName, int nType, const void** ppData, size_t* pnSize, int nIndex = 0 ) const;

    status_t	FindObject( const char* pzName, Flattenable& cVal, int nIndex = 0 ) const;

    status_t	FindMessage( const char* pzName, Message* ppcVal, int nIndex = 0 ) const;
    status_t	FindPointer( const char* pzName, void** ppVal, int nIndex = 0 ) const;
    status_t	FindInt8( const char* pzName, int8* pnVal, int nIndex = 0 ) const;
    status_t	FindInt16( const char* pzName, int16* pnVal, int nIndex = 0 ) const;
    status_t	FindInt32( const char* pzName, int32* pnVal, int nIndex = 0 ) const;
    status_t	FindInt64( const char* pzName, int64* pnVal, int nIndex = 0 ) const;
    status_t	FindBool( const char* pzName, bool* pbVal, int nIndex = 0 ) const;
    status_t	FindFloat( const char* pzName, float* pvVal, int nIndex = 0 ) const;
    status_t	FindDouble( const char* pzName, double* pvVal, int nIndex = 0 ) const;
    status_t	FindRect( const char* pzName, Rect* pcVal, int nIndex = 0 ) const;
    status_t	FindIRect( const char* pzName, IRect* pcVal, int nIndex = 0 ) const;
    status_t	FindPoint( const char* pzName, Point* pcVal, int nIndex = 0 ) const;
    status_t	FindIPoint( const char* pzName, IPoint* pcVal, int nIndex = 0 ) const;
    status_t	FindColor32( const char* pzName, Color32_s* pcVal, int nIndex = 0 ) const;
    status_t	FindString( const char* pzName, const char** pzString, int nIndex = 0 ) const;
    status_t	FindString( const char* pzName, std::string* pcString, int nIndex = 0 ) const;
    status_t	FindString( const char* pzName, String* pcString, int nIndex = 0 ) const;
    status_t	FindVariant( const char* pzName, Variant* pcVal, int nIndex = 0 ) const;
    
    template<class T> status_t FindInt(  const char* pzName, T* pnVal, int nIndex = 0 ) const {
	int32 nValue;
	status_t nError = FindInt32( pzName, &nValue, nIndex );
	if ( nError >= 0 ) {
	    *pnVal = nValue;
	}
	return( nError );
    }
    status_t	RemoveData( const char* pzName, int nIndex = 0 );
    status_t	RemoveName( const char* pzName );

    status_t	GetNameInfo( const char* pzName, int* pnType = NULL, int* pnCount = NULL ) const;
    int		CountNames( void ) const;
    String GetName( int nIndex ) const;
    
    void	MakeEmpty( void );
    bool	IsEmpty( void ) const;

    bool	WasDelivered( void ) const;
    bool	IsSourceWaiting( void ) const;
    bool	IsSourceRemote( void ) const;
    Messenger	ReturnAddress( void ) const;
    bool	IsReply( void ) const;

    status_t	SendReply( int nCode, Handler* pcReplyHandler = NULL );
    status_t	SendReply( Message* pcTheReply, Handler* pcReplyHandler = NULL, uint nTimeOut = ~0 );
    status_t	SendReply( int nCode, Message* pcReplyToReply );
    status_t	SendReply( Message* pcTheReply, Message* pcReplyToReply,
			   int nSendTimOut = ~0, int nReplyTimeOut = ~0 );

    Message& operator=(const Message& cOther );

private:
    friend class Looper;
    friend class MessageQueue;
    friend class Messenger;
    friend class View;
    friend class Settings;

    struct DataArray_s
    {
	DataArray_s*	psNext;
	int		nMaxSize;	// Number of bytes allocated
	int		nCurSize;	// Number of bytes added
	int		nChunkSize;	// Size of each element in a fixed size array, or 0 if the size can vary between elements
	int		nCount;		// Number of elements in array
	int		nTypeCode;	// What kind of data to be found in the array
	uint8		nNameLength;	// Number of bytes in the name string
	  // nNameLength sized name follows
	  // Array of Chunk_s structs (or raw data chunks if fixed size) follows
    };

    struct Chunk_s
    {
	int	nDataSize;
    };
    
    Chunk_s*	 _GetChunkAddr( DataArray_s* psArray, int nIndex )  const;
    uint8*	 _CreateArray(  const char* pzName, int nType, const void* pData, uint32 nSize,
				bool bFixedSize = true, int nMaxCountHint = 1 );
    uint8*	 _ExpandArray( DataArray_s* psArray, const void* pData, uint32 nSize );
  
    void	 _AddArray( DataArray_s* psArray );
    void	 _RemoveArray( DataArray_s* psArray );
    DataArray_s* _FindArray( const char* pzName, int nType ) const;

    void	 _SetReplyHandler( Handler* pcHandler );
    status_t	 _Post( port_id hPort, uint32 nTargetToken, port_id hReplyPort = -1,
			int nReplyTarget = -1, proc_id hReplyProc = -1 );
    status_t	 _ReadPort( port_id hPort, bigtime_t nTimeOut = INFINITE_TIMEOUT );
    int		 _GetStaticSize( void ) const { return( sizeof(m_nCode) + sizeof(m_nFlags) + sizeof(m_nTargetToken) + sizeof(m_nReplyToken) + sizeof(m_hReplyProc) + sizeof(m_hReplyPort) + sizeof( int ) ); }

      // Definitions for m_nFlags
    enum { REPLY_REQUIRED = 0x01, IS_REPLY = 0x02, WAS_DELIVERED = 0x04, REPLY_SENT = 0x00010000 };

    Message*	 m_pcNext;

    int		 m_nCode;
    uint32	 m_nFlags;
    int		 m_nTargetToken;

    int		 m_nReplyToken;
    port_id	 m_hReplyPort;
    proc_id	 m_hReplyProc;
    int		 m_nFlattenedSize;

    DataArray_s* m_psFirstArray;
};

}

#endif	// __F_UTIL_MESSAGE_H__
