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
#include <stdio.h>
#include <limits.h>

#include <storage/filereference.h>
#include <storage/symlink.h>
#include <storage/path.h>

#include <util/exceptions.h>

using namespace os;

class FileReference::Private
{
	public:
    Directory	m_cDirectory;
    String	m_cName;
};

FileReference::FileReference()
{
	m = new Private;
}

FileReference::FileReference( const FileReference & cRef )
{
	m = new Private;
	m->m_cDirectory = cRef.m->m_cDirectory;
	m->m_cName = cRef.m->m_cName;
}

FileReference::FileReference( const String & a_cPath, bool bFollowLinks )
{
	m = new Private;
	
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
	m->m_cName = cPath.GetLeaf();
	if( m->m_cName.size() == 0 )
	{
		throw errno_exception( "Invalid path name", EINVAL );
	}

	if( m->m_cDirectory.SetTo( cPath.GetDir().GetPath(  ), O_RDONLY ) < 0 )
	{
		throw errno_exception( "Failed to open directory" );
	}
}

FileReference::FileReference( const Directory & cDir, const String & cName, bool bFollowLinks )
{
	m = new Private;

	if( m->m_cDirectory.SetTo( cDir ) < 0 )
	{
		throw errno_exception( "Failed to copy directory" );
	}
	m->m_cName = cName;
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
	delete m;
}

status_t FileReference::SetTo( const FileReference & cRef )
{
	if( m->m_cDirectory.SetTo( cRef.m->m_cDirectory ) < 0 )
	{
		return ( -1 );
	}
	m->m_cName = cRef.m->m_cName;
	return ( 0 );
}

int FileReference::SetTo( const String & a_cPath, bool bFollowLinks )
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

int FileReference::SetTo( const Directory & cDir, const String & cName, bool bFollowLinks )
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
	m->m_cDirectory.Unset();
}

bool FileReference::IsValid() const
{
	return ( m->m_cDirectory.IsValid() );
}

String FileReference::GetName() const
{
	return ( m->m_cName );
}

status_t FileReference::GetPath( String * pcPath ) const
{
	String cPath;

	if( m->m_cDirectory.GetPath( &cPath ) < 0 )
	{
		return ( -1 );
	}
	cPath += "/";
	cPath += m->m_cName;
	*pcPath = cPath;
	return ( 0 );
}

status_t FileReference::Rename( const String & cNewName )
{
	String cOldPath;
	status_t nError;

	nError = m->m_cDirectory.GetPath( &cOldPath );
	if( nError < 0 )
	{
		return ( nError );
	}
	cOldPath += "/";
	String cNewPath( cOldPath );
	cOldPath += m->m_cName;

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
			m->m_cName = cNewName;
		}
		else
		{
			SetTo( cNewPath, false );
		}
	}
	return ( 0 );
}

status_t FileReference::Delete()
{
	String cPath;
	if( GetPath( &cPath ) < 0 )
	{
		return ( -1 );
	}
	return ( unlink( cPath.c_str() ) );
}

status_t FileReference::GetStat( struct stat * psStat ) const
{
	int nFD = based_open( m->m_cDirectory.GetFileDescriptor(), m->m_cName.c_str(  ), O_RDONLY );

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
	return ( m->m_cDirectory );
}

