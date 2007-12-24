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

// Provides access to the system name via /etc/hostname.

#ifndef __HOSTNAME_H__
#define __HOSTNAME_H__

#include <util/string.h>

using namespace os;

class Hostname
{
      public:
	Hostname();
	~Hostname();

	const String &GetHostname( void ) const;
	void SetHostname( const String &cHostname );

	const String &GetDomain( void ) const;
	void SetDomain( const String &cDomain );

	void Save( void ) const;
	void Load();

      private:
	String m_cHostname;
	String m_cDomain;

};

#endif
