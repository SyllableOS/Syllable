
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

#include <unistd.h>
#include <stdio.h>
#include <limits.h>

#include <storage/filereference.h>
#include <storage/symlink.h>
#include <storage/path.h>

#include <util/exceptions.h>

using namespace os;

FileReference::FileReference()
{
}

FileReference::FileReference( const FileReference & cRef ):m_cDirectory( cRef.m_cDirectory ), m_cName( cRef.m_cName )
{
}

FileReference::FileReference( const std::string & a_cPath, bool bFollowLinks )
{
	Path cPath( a_cPath.c_str() );

	while( bFollowLinks )
	{
		try
		{
			SymLink cLink( cPath.GetPath() );
			Path cNewPath;

			if( cLink.ConstructPath( cPath.GetDir().GetPath(  ), &cNewPath ) < 0 )
			{
				break;
			}
			cPath = cNewPath;
		}
		catch( ... )
		{
			break;
		}
	}
	m_cName = cPath.GetLeaf();
	if( m_cName.size() == 0 )
	{
		throw errno_exception( "Invalid path name", EINVAL );
	}

	if( m_cDirectory.SetTo( cPath.GetDir().GetPath(  ), O_RDONLY ) < 0 )
	{
		throw errno_exception( "Failed to open directory" );
	}
}

FileReference::FileReference( const Directory & cDir, const std::string & cName, bool bFollowLinks )
{
	if( m_cDirectory.SetTo( cDir ) < 0 )
	{
		throw errno_exception( "Failed to copy directory" );
	}
	m_cName = cName;
	if( bFollowLinks )
	{
		try
		{
			SymLink cLink( cDir, cName );
			Path cNewPath;

			if( cLink.ConstructPath( cDir, &cNewPath ) >= 0 )
			{
				SetTo( cNewPath.GetPath(), true );
			}
		}
		catch( ... )
		{
		}
	}
}

FileReference::~FileReference()
{
}

status_t FileReference::SetTo( const FileReference & cRef )
{
	if( m_cDirectory.SetTo( cRef.m_cDirectory ) < 0 )
	{
		return ( -1 );
	}
	m_cName = cRef.m_cName;
	return ( 0 );
}

int FileReference::SetTo( const std::string & a_cPath, bool bFollowLinks )
{
	try
	{
		FileReference cTmp( a_cPath, bFollowLinks );

		return ( SetTo( cTmp ) );
	}
	catch( errno_exception & cExc )
	{
		errno = cExc.error();
		return ( -1 );
	}
}

int FileReference::SetTo( const Directory & cDir, const std::string & cName, bool bFollowLinks )
{
	try
	{
		FileReference cTmp( cDir, cName, bFollowLinks );

		return ( SetTo( cTmp ) );
	}
	catch( errno_exception & cExc )
	{
		errno = cExc.error();
		return ( -1 );
	}
}

void FileReference::Unset()
{
	m_cDirectory.Unset();
}

bool FileReference::IsValid() const
{
	return ( m_cDirectory.IsValid() );
}

std::string FileReference::GetName() const
{
	return ( m_cName );
}

status_t FileReference::GetPath( std::string * pcPath ) const
{
	std::string cPath;

	if( m_cDirectory.GetPath( &cPath ) < 0 )
	{
		return ( -1 );
	}
	cPath += "/";
	cPath += m_cName;
	*pcPath = cPath;
	return ( 0 );
}

status_t FileReference::Rename( const std::string & cNewName )
{
	std::string cOldPath;
	status_t nError;

	nError = m_cDirectory.GetPath( &cOldPath );
	if( nError < 0 )
	{
		return ( nError );
	}
	cOldPath += "/";
	std::string cNewPath( cOldPath );
	cOldPath += m_cName;

	if( cNewName[0] == '/' )
	{
		cNewPath = cNewName;
	}
	else
	{
		cNewPath += cNewName;
	}
	nError = rename( cOldPath.c_str(), cNewPath.c_str(  ) );
	if( nError < 0 )
	{
		return ( nError );
	}
	else
	{
		if( strchr( cNewName.c_str(), '/' ) == NULL )
		{
			m_cName = cNewName;
		}
		else
		{
			SetTo( cNewPath, false );
		}
	}
	return ( -1 );
}

status_t FileReference::Delete()
{
	std::string cPath;
	if( GetPath( &cPath ) < 0 )
	{
		return ( -1 );
	}
	return ( unlink( cPath.c_str() ) );
}

status_t FileReference::GetStat( struct::stat * psStat ) const
{
	int nFD = based_open( m_cDirectory.GetFileDescriptor(), m_cName.c_str(  ), O_RDONLY );

	if( nFD < 0 )
	{
		return ( -1 );
	}
	if( fstat( nFD, psStat ) < 0 )
	{
		close( nFD );
		return ( -1 );
	}
	else
	{
		close( nFD );
		return ( 0 );
	}
}

const Directory & FileReference::GetDirectory() const
{
	return ( m_cDirectory );
}
