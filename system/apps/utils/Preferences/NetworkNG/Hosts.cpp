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

#include "Hosts.h"
#include "StringWriter.h"
#include "StringReader.h"

#include <storage/file.h>

#define HOSTS_FILE "/etc/hosts"

Hosts::Hosts()
{
	Load();
}

Hosts::~Hosts()
{
}

void Hosts::GetHostEntries( HostEntryList_t & cEntries )
{
	cEntries.clear();
	for( HostEntryList_t::iterator i = m_cEntries.begin(); i != m_cEntries.end(  ); i++ )
	{

		cEntries.push_back( *i );
	}
}

void Hosts::AddHostEntry( const String &cAddress, const String &cAliases )
{
	m_cEntries.push_back( HostEntry( cAddress, cAliases ) );
}

void Hosts::DeleteHostEntry( const String &cAddress )
{
	for( HostEntryList_t::iterator i = m_cEntries.begin(); i != m_cEntries.end(  ); i++ )
	{

		if( ( *i ).Address == cAddress )
		{
			m_cEntries.erase( i );
			break;
		}
	}
}

void Hosts::Load( void )
{
	File cFile;

	m_cEntries.clear();

	if( cFile.SetTo( HOSTS_FILE ) == 0 )
	{
		StringReader cSr( &cFile );
		std::vector < String >cLines;

		cLines.push_back( cSr.ReadLine() );
		while( cSr.HasLines() )
		{
			cLines.push_back( cSr.ReadLine() );
		}

		for( std::vector < String >::iterator i = cLines.begin(); i != cLines.end(  ); i++ )
		{

			if( ( *i ).Length() > 0 )
			{
				for( uint32 nPos = 0; nPos < ( *i ).Length(); nPos++ )
					if( ( *i )[nPos] == ' ' || ( *i )[nPos] == '\t' )
					{
						m_cEntries.push_back( HostEntry( ( *i ).substr( 0, nPos ).Strip(), ( *i ).substr( nPos + 1 ).Strip() ) );
						break;
					}
			}
		}
	}
}

void Hosts::Save( void )
{
	File cFile;

	if( cFile.SetTo( HOSTS_FILE, O_CREAT | O_TRUNC ) == 0 )
	{
		StringWriter cSw( &cFile );

		for( HostEntryList_t::iterator i = m_cEntries.begin(); i != m_cEntries.end(  ); i++ )
		{
			cSw.Write( ( *i ).Address );
			cSw.Write( '\t' );
			cSw.WriteLine( ( *i ).Aliases );
		}
	}
}

