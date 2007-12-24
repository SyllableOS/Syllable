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

#include "StringReader.h"

StringReader::StringReader( StreamableIO * pcStream )
{
	m_pcStream = pcStream;
}

StringReader::~StringReader()
{

}

String StringReader::ReadLine( void )
{
	// Initialise with the previous partial line.
	String cStr = m_cPartLine;

	// Buffer for reading more data.
	char buf[0x400];

	// Do we already have a complete line?
	int nPos = cStr.find( "\n" );
	int nBytes = 1;

	// Loop to read more data.
	while( nPos == String::npos && nBytes > 0 )
	{
		// Read bytes
		nBytes = m_pcStream->Read( ( void * )buf, 0x400 - 1 );
		if( nBytes > 0 )
		{
			// Append read bytes to string.
			buf[nBytes] = 0;
			cStr += ( const char * )&buf;
		}
		// Look for a new line.
		nPos = cStr.find( "\n" );
	}
	if( nPos != String::npos )
	{
		// Grab the complete line to return.
		String cFound = cStr.substr( 0, nPos );

		// Store the rest of the string for the next call.
		m_cPartLine = cStr.substr( nPos + 1 );
		m_bHasLines = true;
		return cFound;
	}
	else
	{
		// No line found. We have read to the end of the stream.
		m_cPartLine = "";
		m_bHasLines = false;
		return cStr;
	}
}

String StringReader::ReadToEnd( void )
{
	String cStr;
	char buf[0x8000];
	int nBytes = m_pcStream->Read( ( void * )buf, 0x8000 );

	while( nBytes > 0 )
	{
		buf[nBytes] = 0;
		cStr += ( const char * )&buf;
		nBytes = m_pcStream->Read( ( void * )buf, 0x8000 - 1 );
	}

	return cStr;
}

bool StringReader::HasLines( void )
{
	return m_bHasLines;
}
