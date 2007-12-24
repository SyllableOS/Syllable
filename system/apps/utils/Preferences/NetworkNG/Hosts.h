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

// Provides access to entries in the /etc/hosts file.

#ifndef __HOSTS_H__
#define __HOSTS_H__

#include <util/string.h>
#include <vector>

using namespace os;

struct HostEntry
{
	HostEntry( const String &cAddress, const String &cAliases )
	{
		Address = cAddress;
		  Aliases = cAliases;
	}

	String Address;
	String Aliases;
};

typedef std::vector < HostEntry > HostEntryList_t;

class Hosts
{
      public:
	Hosts();
	~Hosts();

	void GetHostEntries( HostEntryList_t & cEntries );
	void AddHostEntry( const String &cAddress, const String &cAliases );
	void DeleteHostEntry( const String &cAddress );

	void Load( void );
	void Save( void );

      private:
	HostEntryList_t m_cEntries;
};

#endif
