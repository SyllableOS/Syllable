/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#ifndef __F_UTIL_REGEXP_H__
#define __F_UTIL_REGEXP_H__

#include <storage/file.h>
#include <atheos/image.h>
#include <util/string.h>

#include <vector>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif

/** Regular expression class.
 * \ingroup util
 * \par Description:
 *	The os::RegExp class allow you to do regular expression searches
 *	on strings and to extract sub-strings from the searched string.
 *	The resulting sub-strings from a search can also be used to
 *	expand strings containing sub-expression references.
 * \since 0.3.7
 * \sa os::String
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class RegExp
{
public:
    enum {
	ERR_BADREPEAT		= -2,
	ERR_BADBACKREF		= -3,
	ERR_BADBRACE		= -4,
	ERR_BADBRACK		= -5,
	ERR_BADPARENTHESIS	= -6,
	ERR_BADRANGE		= -7,
	ERR_BADSUBREG		= -8,
	ERR_BADCHARCLASS	= -9,
	ERR_BADESCAPE		= -10,
	ERR_BADPATTERN		= -11,
	ERR_TOLARGE		= -12,
	ERR_NOMEM		= -13,
	ERR_GENERIC		= -14
    };
    class exception : public std::exception {
    public:
	exception( int nError ) { error = nError; }
	int error;
    };
public:
    RegExp();
    RegExp( const String& cExpression, bool bNoCase = false, bool bExtended = false );
    ~RegExp();

    status_t Compile( const String& cExpression, bool bNoCase = false, bool bExtended = false );
    int	     GetSubExprCount() const;
    bool IsValid() const;
    
    bool Search( const String& cString );
    bool Search( const String& cString, int nStart, int nLen = -1 );
    bool Match( const String& cString );
    bool Match( const String& cString, int nStart, int nLen = -1 );

    String Expand( const String& cPattern ) const;
    
    int GetStart() const;
    int GetEnd() const;
    
    const String& GetSubString( uint nIndex ) const;
    bool	       GetSubString( uint nIndex, int* pnStart, int* pnEnd ) const;
    const std::vector<String>& GetSubStrList() const;
private:
    class Private;
    Private* m;
};

} // end of namespace

#endif // __F_UTIL_REGEXP_H__
