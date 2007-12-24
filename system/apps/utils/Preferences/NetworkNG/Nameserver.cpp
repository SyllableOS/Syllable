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

#include "Nameserver.h"
#include "StringWriter.h"
#include "StringReader.h"

#include <storage/file.h>

#define NAMESERVER_FILE "/etc/resolv.conf"

Nameserver::Nameserver()
{
	Load();
}

Nameserver::~Nameserver()
{
}

void Nameserver::GetDomains( DomainList_t & cDomains )
{
	cDomains.clear();
	for( DomainList_t::iterator i = m_cDomains.begin(); i != m_cDomains.end(  ); i++ )
	{

		cDomains.push_back( *i );
	}
}

void Nameserver::AddDomain( const String &cDomain )
{
	m_cDomains.push_back( cDomain );
}

void Nameserver::DeleteDomain( const String &cDomain )
{
	for( DomainList_t::iterator i = m_cDomains.begin(); i != m_cDomains.end(  ); i++ )
	{
		if( *i == cDomain )
		{
			m_cDomains.erase( i );
			break;
		}
	}
}

void Nameserver::GetNameservers( NameserverList_t & cServers )
{
	cServers.clear();
	for( NameserverList_t::iterator i = m_cServers.begin(); i != m_cServers.end(  ); i++ )
	{

		cServers.push_back( *i );
	}
}

void Nameserver::AddNameserver( const String &cServer )
{
	m_cServers.push_back( cServer );
}

void Nameserver::DeleteNameserver( const String &cServer )
{
	for( NameserverList_t::iterator i = m_cServers.begin(); i != m_cServers.end(  ); i++ )
	{
		if( *i == cServer )
		{
			m_cServers.erase( i );
			break;
		}
	}
}


void Nameserver::Save( void )
{
	File cFile;

	if( cFile.SetTo( NAMESERVER_FILE, O_CREAT | O_TRUNC ) == 0 )
	{
		StringWriter cSw( &cFile );

		for( DomainList_t::iterator i = m_cDomains.begin(); i != m_cDomains.end(  ); i++ )
		{
			cSw.Write( "search " );
			cSw.WriteLine( *i );
		}

		for( NameserverList_t::iterator i = m_cServers.begin(); i != m_cServers.end(  ); i++ )
		{
			cSw.Write( "nameserver " );
			cSw.WriteLine( *i );
		}
	}
}


void Nameserver::Load( void )
{
	File cFile;

	m_cDomains.clear();
	m_cServers.clear();

	if( cFile.SetTo( NAMESERVER_FILE ) == 0 )
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

			if( ( *i ).Length() > 7 && ( *i ).substr( 0, 7 ) == "search " )
			{
				m_cDomains.push_back( ( *i ).substr( 7 ) );
			}

			if( ( *i ).Length() > 11 && ( *i ).substr( 0, 11 ) == "nameserver " )
			{
				m_cServers.push_back( ( *i ).substr( 11 ) );
			}
		}
	}
}
