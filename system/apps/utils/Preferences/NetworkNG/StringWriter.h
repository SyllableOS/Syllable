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


#ifndef __STRINGWRITER_H__
#define __STRINGWRITER_H__

#include <storage/streamableio.h>
#include <util/string.h>
#include <stdarg.h>

using namespace os;

// Utility class for writing strings to streams.
class StringWriter
{
      public:
	StringWriter( StreamableIO * pcStream );
	~StringWriter();

	void Write( const String &cString );
	void Write( char nChar );
	void WriteLine( const String &cString );

      private:
	  StreamableIO * m_pcStream;
};

#endif
