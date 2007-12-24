// Syllable Network Preferences - Copyright 2006 Andrew Kennan
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// Provides access to the system name via the /etc/hostname file.

#include <storage/file.h>
#include <storage/filereference.h>

#include "Hostname.h"
#include "StringWriter.h"
#include "StringReader.h"

// File to read and write.
#define HOSTNAME_FILE "/etc/hostname"

Hostname::Hostname()
{
	Load();
}

Hostname::~Hostname()
{
}

const String &Hostname::GetHostname( void ) const
{
	return m_cHostname;
}

void Hostname::SetHostname( const String &cHostname )
{
	m_cHostname = cHostname;
}

const String &Hostname::GetDomain( void ) const
{
	return m_cDomain;
}

void Hostname::SetDomain( const String &cDomain )
{
	m_cDomain = cDomain;
}

void Hostname::Save( void ) const
{
	File cFile;

	if( cFile.SetTo( HOSTNAME_FILE, O_CREAT | O_TRUNC ) == 0 )
	{
		StringWriter cSw( &cFile );

		cSw.Write( m_cHostname );
		if( m_cDomain.Length() > 0 )
		{
			cSw.Write( '.' );
			cSw.Write( m_cDomain );
		}
		cSw.WriteLine( "" );
	}
}

void Hostname::Load( void )
{
	File cFile;

	if( cFile.SetTo( HOSTNAME_FILE ) == 0 )
	{
		StringReader cSr( &cFile );
		String cTmp = cSr.ReadToEnd();
		int nPos = cTmp.find( "." );

		if( nPos != String::npos )
		{
			m_cHostname = cTmp.substr( 0, nPos ).Strip();
			m_cDomain = cTmp.substr( nPos + 1 ).Strip();
		}
		else
		{
			m_cHostname = cTmp.Strip();
			m_cDomain = "";
		}
	}
}
