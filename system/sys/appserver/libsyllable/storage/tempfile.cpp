
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
#include <storage/tempfile.h>

using namespace os;

class TempFile::Private
{
	public:
	String m_cPath;
	bool m_bDeleteFile;
};


TempFile::TempFile( const String& cPrefix, const String& cPath, int nAccess )
:File()
{
	m = new Private;
	m->m_bDeleteFile = true;
	while( true )
	{
		m->m_cPath = tempnam( cPath.c_str(), cPrefix.c_str() );
		if( SetTo( m->m_cPath, O_RDWR | O_CREAT | O_EXCL ) < 0 )
		{
			if( errno == EEXIST )
			{
				continue;
			}
			else
			{
				throw errno_exception( "Failed to create file" );
			}
		}
		else
		{
			break;
		}
	}
}

TempFile::~TempFile()
{
	if( m->m_bDeleteFile )
	{
		unlink( m->m_cPath.c_str() );
	}
	delete m;
}

void TempFile::Detatch()
{
	m->m_bDeleteFile = false;
}

status_t TempFile::Unlink()
{
	if( m->m_bDeleteFile )
	{
		m->m_bDeleteFile = false;
		return ( unlink( m->m_cPath.c_str() ) );
	}
	else
	{
		return ( 0 );
	}
}

String TempFile::GetPath() const
{
	return ( m->m_cPath );
}


