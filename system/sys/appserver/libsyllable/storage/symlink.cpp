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
#include <limits.h>
#include <storage/symlink.h>
#include <storage/filereference.h>
#include <storage/path.h>

#include <util/exceptions.h>
using namespace os;

SymLink::SymLink()
{
}

SymLink::SymLink( const String & cPath, int nOpenMode ):FSNode( cPath, nOpenMode | O_NOTRAVERSE )
{
	if( S_ISLNK( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node not a symlink", EINVAL );
	}
}

SymLink::SymLink( const Directory & cDir, const String & cName, int nOpenMode ):FSNode( cDir, cName, nOpenMode | O_NOTRAVERSE )
{
	if( S_ISLNK( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node not a symlink", EINVAL );
	}
}

SymLink::SymLink( const FileReference & cRef, int nOpenMode ):FSNode( cRef, nOpenMode | O_NOTRAVERSE )
{
	if( S_ISLNK( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node not a symlink", EINVAL );
	}
}

SymLink::SymLink( const FSNode & cNode ):FSNode( cNode )
{
	if( S_ISLNK( GetMode( false ) ) == false )
	{
		throw errno_exception( "Node not a symlink", EINVAL );
	}
}

SymLink::SymLink( const SymLink & cLink ):FSNode( cLink )
{
}

SymLink::~SymLink()
{
}

status_t SymLink::SetTo( const String & cPath, int nOpenMode )
{
	try
	{
		return ( SetTo( FSNode( cPath, nOpenMode | O_NOTRAVERSE ) ) );
	}
	catch( errno_exception & cExc )
	{
		errno = cExc.error();
		return ( -1 );
	}
}

status_t SymLink::SetTo( const Directory & cDir, const String & cPath, int nOpenMode )
{
	try
	{
		return ( SetTo( FSNode( cDir, cPath, nOpenMode | O_NOTRAVERSE ) ) );
	}
	catch( errno_exception & cExc )
	{
		errno = cExc.error();
		return ( -1 );
	}
}

status_t SymLink::SetTo( const FileReference & cRef, int nOpenMode )
{
	try
	{
		return ( SetTo( FSNode( cRef, nOpenMode | O_NOTRAVERSE ) ) );
	}
	catch( errno_exception & cExc )
	{
		errno = cExc.error();
		return ( -1 );
	}
}

status_t SymLink::SetTo( const FSNode & cNode )
{
	if( S_ISLNK( cNode.GetMode( false ) ) == false )
	{
		errno = EINVAL;
		return ( -1 );
	}
	return ( FSNode::SetTo( cNode ) );
}

status_t SymLink::SetTo( const SymLink & cLink )
{
	return ( FSNode::SetTo( cLink ) );
}

status_t SymLink::ReadLink( String * pcBuffer )
{
	String cBuffer;
	int nSize = PATH_MAX;

//	cBuffer.resize( PATH_MAX );
	for( ;; )
	{
		char* pzBfr = new char[ nSize ];
		ssize_t nLen = freadlink( GetFileDescriptor(), pzBfr, nSize );
//		ssize_t nLen = freadlink( GetFileDescriptor(), cBuffer.begin( ), cBuffer.size(  ) );
		cBuffer = pzBfr;
		delete []pzBfr;

		if( nLen == ssize_t ( nSize ) )
		{
			nSize*=2;
//			cBuffer.resize( cBuffer.size() * 2 );
		}
		else if( nLen < 0 )
		{
			return ( -1 );
		}
		else
		{
			cBuffer.resize( nLen );
			break;
		}
	}
	*pcBuffer = cBuffer;
	return ( 0 );
}

String SymLink::ReadLink()
{
	String cBuffer;
	if( ReadLink( &cBuffer ) >= 0 )
	{
		return ( cBuffer );
	}
	else
	{
		return ( "" );
	}
}

status_t SymLink::ConstructPath( const String & cParent, Path * pcPath )
{
	String cBuffer;
	if( ReadLink( &cBuffer ) < 0 )
	{
		return ( -1 );
	}
	if( cBuffer[0] == '/' )
	{
		*pcPath = cBuffer.c_str();
	}
	else
	{
		*pcPath = cParent.c_str();
		pcPath->Append( cBuffer.c_str() );
	}
	return ( 0 );
}

status_t SymLink::ConstructPath( const Directory & cParent, Path * pcPath )
{
	String cParentPath;
	if( cParent.GetPath( &cParentPath ) < 0 )
	{
		return ( -1 );
	}
	return ( ConstructPath( cParentPath, pcPath ) );
}
