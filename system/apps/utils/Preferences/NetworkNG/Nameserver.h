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

#ifndef __NAMESERVER_H__
#define __NAMESERVER_H__

#include <util/string.h>
#include <vector>

using namespace os;

typedef std::vector < String >DomainList_t;
typedef std::vector < String >NameserverList_t;

// Provides access to resolv.conf
class Nameserver
{
      public:
	Nameserver();
	~Nameserver();

	void GetDomains( DomainList_t & cDomains );
	void AddDomain( const String &cDomain );
	void DeleteDomain( const String &cDomain );

	void GetNameservers( NameserverList_t & cServers );
	void AddNameserver( const String &cServer );
	void DeleteNameserver( const String &cServer );

	void Save( void );
	void Load( void );

      private:
	DomainList_t m_cDomains;
	NameserverList_t m_cServers;
};

#endif
