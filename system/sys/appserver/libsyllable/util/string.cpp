
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
#include <ctype.h>

#include <util/string.h>
#include <gui/font.h>

using namespace os;



String::String ()
{
}

String::String ( int nLen, char nFiller ):m_cString( nLen, nFiller )
{
}

String::String ( const char *pzString ):m_cString( pzString )
{
}

String::String ( const char *pzString, int nLen ):m_cString( pzString, nLen )
{
}

String::String ( const std::string & cString ):m_cString( cString )
{
}

String::String ( const std::string & cString, int nPos, int nLen ):m_cString( cString, nPos, nLen )
{
}

String::String ( const String &cString ):m_cString( cString.m_cString )
{
}

String::String ( const String &cString, int nPos, int nLen ):m_cString( cString.m_cString, nPos, nLen )
{
}

String::String ( const_iterator cBegin, const_iterator cEnd ):m_cString( cBegin, cEnd )
{
}


const char *String::c_str() const
{
	return ( m_cString.c_str() );
}

size_t String::Length() const
{
	return ( m_cString.size() );
}

size_t String::CountChars() const
{
	int nNumChars = 0;

	for( uint i = 0; i < m_cString.size(); i += utf8_char_length( m_cString[i] ) )
	{
		nNumChars++;
	}
	return ( nNumChars );
}

String &String::String::Resize( int nNewLen )
{
	m_cString.resize( nNewLen );
	return ( *this );
}


String &String::Format( const char *pzFormat, va_list pArgs )
{
	int nMaxLen = 4096;
	char zBuffer[nMaxLen];
	int nLen = vsnprintf( zBuffer, nMaxLen, pzFormat, pArgs );

	if( nLen < nMaxLen )
	{
		m_cString = zBuffer;
		return ( *this );
	}
	while( nLen >= nMaxLen )
	{
		nMaxLen *= 2;
		char *pzBuffer = new char[nMaxLen];

		nLen = vsnprintf( pzBuffer, nMaxLen, pzFormat, pArgs );
		if( nLen < nMaxLen )
		{
			m_cString = pzBuffer;
		}
		delete[]pzBuffer;
	}
	return ( *this );
}

String &String::Format( const char *pzFormat, ... )
{
	va_list sArgList;

	va_start( sArgList, pzFormat );
	Format( pzFormat, sArgList );
	va_end( sArgList );
	return ( *this );
}

String &String::Strip()
{
	RStrip();
	LStrip();
	return ( *this );
}

String &String::LStrip()
{
	std::string::iterator i;

	for( i = m_cString.begin(); i != m_cString.end(  ); ++i )
	{
		if( isspace( *i ) == false )
		{
			break;
		}
	}
	m_cString.erase( m_cString.begin(), i );
	return ( *this );
}

String &String::RStrip()
{
	uint nSpaces = 0;

	for( uint i = m_cString.size() - 1; i >= 0; --i )
	{
		if( isspace( m_cString[i] ) == false )
		{
			break;
		}
		nSpaces++;
	}
	m_cString.resize( m_cString.size() - nSpaces );
	return ( *this );
}

String &String::Lower()
{
	for( uint i = 0; i < m_cString.size(); ++i )
	{
		m_cString[i] = tolower( m_cString[i] );
	}
	return ( *this );
}

String &String::Upper()
{
	for( uint i = 0; i < m_cString.size(); ++i )
	{
		m_cString[i] = toupper( m_cString[i] );
	}
	return ( *this );
}

int String::Compare( const char *pzString ) const
{
	return ( strcmp( m_cString.c_str(), pzString ) );
}

int String::Compare( const std::string & cOther ) const
{
	return ( strcmp( m_cString.c_str(), cOther.c_str(  ) ) );
}

int String::Compare( const String &cOther ) const
{
	return ( strcmp( m_cString.c_str(), cOther.m_cString.c_str(  ) ) );
}


int String::CompareNoCase( const char *pzString ) const
{
	return ( strcasecmp( m_cString.c_str(), pzString ) );
}

int String::CompareNoCase( const std::string & cOther ) const
{
	return ( strcasecmp( m_cString.c_str(), cOther.c_str(  ) ) );
}

int String::CompareNoCase( const String &cOther ) const
{
	return ( strcasecmp( m_cString.c_str(), cOther.m_cString.c_str(  ) ) );
}


String &String::operator=( const char *pzString )
{
	m_cString = pzString;
	return ( *this );
}

String &String::operator=( const std::string & cString )
{
	m_cString = cString;
	return ( *this );
}

String &String::operator=( const String &cString )
{
	m_cString = cString.m_cString;
	return ( *this );
}


String &String::operator+=( const char *pzString )
{
	m_cString += pzString;
	return ( *this );
}

String &String::operator+=( const char nChar )
{
	m_cString += nChar;
	return ( *this );
}

String &String::operator+=( const std::string & cString )
{
	m_cString += cString;
	return ( *this );
}

String &String::operator+=( const String &cString )
{
	m_cString += cString.m_cString;
	return ( *this );
}

String String::operator+( const char *pzString ) const
{
	String cStr( *this );

	cStr += pzString;
	return ( cStr );
}

String String::operator+( const char nChar ) const
{
	String cStr( *this );

	cStr += nChar;
	return ( cStr );
}

String String::operator+( const std::string & cString ) const
{
	String cStr( *this );

	cStr.m_cString += cString;
	return ( cStr );
}

String String::operator+( const String &cString ) const
{
	String cStr( *this );

	cStr.m_cString += cString.m_cString;
	return ( cStr );
}

bool String::operator==( const char *pzString ) const
{
	return ( m_cString == pzString );
}

bool String::operator==( const std::string & cString ) const
{
	return ( m_cString == cString );
}

bool String::operator==( const String &cString ) const
{
	return ( m_cString == cString.m_cString );
}

bool String::operator!=( const char *pzString ) const
{
	return ( m_cString != pzString );
}

bool String::operator!=( const std::string & cString ) const
{
	return ( m_cString != cString );
}

bool String::operator!=( const String &cString ) const
{
	return ( m_cString != cString.m_cString );
}

bool String::operator<( const char *pzString ) const
{
	return ( m_cString < pzString );
}

bool String::operator<( const std::string & cString ) const
{
	return ( m_cString < cString );
}

bool String::operator<( const String &cString ) const
{
	return ( m_cString < cString.m_cString );
}

bool String::operator>( const char *pzString ) const
{
	return ( m_cString > pzString );
}

bool String::operator>( const std::string & cString ) const
{
	return ( m_cString > cString );
}

bool String::operator>( const String &cString ) const
{
	return ( m_cString > cString.m_cString );
}

char String::operator[] ( size_t nPos )
    const
    {
	    return ( m_cString[nPos] );
    }

    char &String::operator[] ( size_t nPos )
{
	return ( m_cString[nPos] );
}

/*
String::operator std::string&()
{
    return( m_cString );
}
*/
String::operator  const std::string & () const
{
    return ( m_cString );
}

std::string & String::str()
{
	return ( m_cString );
}

const std::string & String::const_str() const
{
	return ( m_cString );
}

String::iterator String::begin()
{
	return ( m_cString.begin() );
}
String::iterator String::end()
{
	return ( m_cString.end() );
}

String::const_iterator String::begin() const
{
	return ( m_cString.begin() );
}

String::const_iterator String::end() const
{
	return ( m_cString.end() );
}

size_t String::size() const
{
	return ( m_cString.size() );
}

bool String::empty() const
{
	return ( m_cString.empty() );
}

void String::resize( size_t nLen )
{
	m_cString.resize( nLen );
}

void String::resize( size_t nLen, char nFiller )
{
	m_cString.resize( nLen, nFiller );
}

void String::reserve( size_t nLen )
{
	m_cString.reserve( nLen );
}


String &String::erase( size_t nPos, size_t nLen )
{
	m_cString.erase( nPos, nLen );
	return ( *this );
}

String::iterator String::erase( iterator i )
{
	return ( m_cString.erase( i ) );
}

String::iterator String::erase( iterator cFirst, iterator cLast )
{
	return ( m_cString.erase( cFirst, cLast ) );
}

String String::substr( size_t nPos, size_t nLen ) const
{
	return ( m_cString.substr( nPos, nLen ) );
}

size_t	String::find( const String& cStr, size_t nPos ) const
{
	return ( m_cString.find( cStr, nPos ) );
}
