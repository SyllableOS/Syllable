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

#ifndef __F_UTIL_STRING_H__
#define __F_UTIL_STRING_H__

#include <string>
#include <stdarg.h>

namespace os
{
#if 0
}
#endif


/** String manipulation class.
 * \ingroup util
 * \par Description:
 *	The os::String class let you store and manipulate UTF8 strings and
 *	can be used interchangably with the std::string class.
 *
 *	It use an std::string object for storage so casting an os::String
 *	to a std::string referrence is very efficient. It simply return a
 *	reference to the internal std::string.
 *
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class String
{
public:
    typedef std::string::iterator iterator;
    typedef std::string::const_iterator const_iterator;
    enum { npos = -1 };
public:
    String();
//    String( uint32 nID, Catalog *pcCatalog = NULL );
    String( int nLen, char nFiller );
    String( const char* pzString );
    String( const char* pzString, int nLen );
    String( const std::string& cString );
    String( const std::string& cString, int nPos, int nLen );
    String( const String& cString );
    String( const String& cString, int nPos, int nLen );
    String( const_iterator cBegin, const_iterator cEnd );
    
    size_t	Length() const;
    size_t	CountChars() const;
    
    String& Resize( int nNewLen );

    
    String& Format( const char* pzFormat, va_list pArgs );
    String& Format( const char* pzFormat, ... );

    String& Strip();
    String& LStrip();
    String& RStrip();

    String& Lower();
    String& Upper();
    
    int	Compare( const char* pzString ) const;
    int	Compare( const std::string& cOther ) const;
    int	Compare( const String& cOther ) const;

    int CompareNoCase( const char* pzString ) const;
    int CompareNoCase( const std::string& cOther ) const;
    int CompareNoCase( const String& cOther ) const;


    String& operator=( const char* pzString );
    String& operator=( const std::string& cString );
    String& operator=( const String& cString );
    
    String& operator+=( const char* pzString );
    String& operator+=( const char nChar );
    String& operator+=( const std::string& cString );
    String& operator+=( const String& cString );
    
    String operator+( const char* pzString ) const;
	String operator+( const char nChar ) const;
    String operator+( const std::string& cString ) const;
    String operator+( const String& cString ) const;

    bool operator==( const char* pzString ) const;
    bool operator==( const std::string& cString ) const;
    bool operator==( const String& cString ) const;

    bool operator!=( const char* pzString ) const;
    bool operator!=( const std::string& cString ) const;
    bool operator!=( const String& cString ) const;

    bool operator<( const char* pzString ) const;
    bool operator<( const std::string& cString ) const;
    bool operator<( const String& cString ) const;

    bool operator>( const char* pzString ) const;
    bool operator>( const std::string& cString ) const;
    bool operator>( const String& cString ) const;

    char  operator[]( size_t nPos ) const;
    char& operator[]( size_t nPos );
    
    operator const std::string&() const;
    std::string& str();
    const std::string& const_str() const;

public: // std::string proxys    
    const char* c_str() const;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    size_t	   size() const;
    bool	   empty() const;
    void	   resize( size_t nLen );
    void	   resize( size_t nLen, char nFiller );
    void	   reserve( size_t nLen );

    String& 	erase( size_t nPos = 0, size_t nLen = npos );
    iterator 	erase( iterator i );
    iterator 	erase( iterator cFirst, iterator cLast );

	String		substr( size_t nPos = 0, size_t nLen = npos ) const;

    size_t		find( const String& cStr, size_t nPos = 0 ) const;

private:
    std::string m_cString;
};

}

#endif // __UTIL_STRING_H__
