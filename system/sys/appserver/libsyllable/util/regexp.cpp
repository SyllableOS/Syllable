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

#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include <regex.h>

#include <util/regexp.h>

#include <vector>

using namespace os;


class RegExp::Private
{
      public:
	regex_t m_sRegex;
	regmatch_t *m_pasMatches;
	std::vector < String > m_cSubStrings;
	int m_nStartPos;
	bool m_bValid;
};

static status_t conv_error( status_t nError )
{
	switch ( nError )
	{
	case 0:
		return ( 0 );
	case REG_BADRPT:
		return ( RegExp::ERR_BADREPEAT );
	case REG_BADBR:
		return ( RegExp::ERR_BADBACKREF );
	case REG_EBRACE:
		return ( RegExp::ERR_BADBRACE );
	case REG_EBRACK:
		return ( RegExp::ERR_BADBRACK );
	case REG_ERANGE:
		return ( RegExp::ERR_BADRANGE );
	case REG_ECTYPE:
		return ( RegExp::ERR_BADCHARCLASS );
	case REG_EPAREN:
		return ( RegExp::ERR_BADPARENTHESIS );
	case REG_ESUBREG:
		return ( RegExp::ERR_BADSUBREG );
	case REG_EEND:
		return ( RegExp::ERR_GENERIC );
	case REG_EESCAPE:
		return ( RegExp::ERR_BADESCAPE );
	case REG_BADPAT:
		return ( RegExp::ERR_BADPATTERN );
	case REG_ESIZE:
		return ( RegExp::ERR_TOLARGE );
	case REG_ESPACE:
		return ( RegExp::ERR_NOMEM );
	default:
		return ( RegExp::ERR_GENERIC );
	}
}

/** Default constructor.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

RegExp::RegExp()
{
	m = new Private;
	m->m_bValid = false;
}

/** Constructor
 * \par Description:
 *	Construct the os::RegExp object and compile the given pattern.
 *	See Compile() for a more detailed description of the
 *	pattern compilation mechanism.
 * \par
 *	If the regular expression is invalid an os::RegExp::exception
 *	exception will be thrown. The "error" member of the exception
 *	will be set to one of the os::RegExp::ERR_* error codes.
 * \param cExpression
 *	The regular expression to compile.
 * \param bNoCase
 *	Set to \b true if case should be ignored when doing subsequent searches.
 * \param bExtended
 *	If \b true the POSIX Extended Regular Expression Syntax will be
 *	used. If \b false the POSIX Basic Regular Expression Syntax is
 *	used.
 * \sa Compile()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

RegExp::RegExp( const String & cExpression, bool bNoCase, bool bExtended )
{
	m = new Private;
	m->m_pasMatches = NULL;
	m->m_bValid = false;
	status_t nError = Compile( cExpression, bNoCase, bExtended );

	if( nError != 0 )
	{
		throw( exception( nError ) );
	}
}

/** Destructor.
 * \par Description:
 *	Destructor.
 * \par Note:
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

RegExp::~RegExp()
{
	if( m->m_bValid )
	{
		regfree( &m->m_sRegex );
		delete[]m->m_pasMatches;
	}
	delete m;
}

/** Compile an regular expression.
 * \par Description:
 *	Compile a regular expression into a form that is suitable for
 *	subsequent searches and matches.
 * \par
 *	The regular expression can be interpreted in two distinct ways
 *	If the \p bExtended argument is false (the default) the POSIX
 *	Basic Regular Expression Syntax is assumed, if it is true the
 *	the POSIX Extended Regular Expression Syntax is assumed.
 * \par
 *	If the \p bNoCase argument is false subsequent searches will be
 *	case insensitive.
 * \par
 *	If there is an syntactical error in the expression one of the
 *	ERR_* error codes will be returned:
 * \par
 *	<table><tr><td>Error Code</td><td>Description</td></tr>
 *	<tr><td> ERR_BADREPEAT      </td><td> An invalid repetition operator such as '*' or '?' appeared
 *					      in a bad position (with no preceding subexpression to act on).</td></tr>
 *	<tr><td> ERR_BADBACKREF     </td><td> There was an invalid backreference (\\{...\\}) construct in the expression.
 *					      A valid backreference contain either a single numbed or two numbers
 *					      in increasing order separated by a comma.</td></tr>
 *	<tr><td> ERR_BADBRACE       </td><td> The expression had an unbalanced \\{ \\} construct.</td></tr>
 *	<tr><td> ERR_BADBRACK       </td><td> The expression had an unbalanced \\[ \\] construct.</td></tr>
 *	<tr><td> ERR_BADPARENTHESIS </td><td> The expression had an unbalanced \\( \\) construct.</td></tr>
 *	<tr><td> ERR_BADRANGE       </td><td> One of the endpoints in a range expression was invalid./td></tr>
 *	<tr><td> ERR_BADSUBREG      </td><td> There was an invalid number in a \\digit construct.</td></tr>
 *	<tr><td> ERR_BADCHARCLASS   </td><td> The expression referred to an invalid character class name.</td></tr>
 *	<tr><td> ERR_BADESCAPE      </td><td> Invalid escape sequence (the expression ended with an '\\').</td></tr>
 *	<tr><td> ERR_BADPATTERN     </td><td> There was an syntax error in the regular expression.</td></tr>
 *	<tr><td> ERR_TOLARGE        </td><td> The expression was to large. Compiled regular expression buffer
 *					      requires a pattern buffer larger than 64Kb.</td></tr>
 *	<tr><td> ERR_NOMEM          </td><td> Ran out of memory while compiling the expression.</td></tr>
 *	<tr><td> ERR_GENERIC        </td><td> Unspecified error.</td></tr>
 *	</table>
 *
 * \param cExpression
 *	The regular expression to compile.
 * \param bNoCase
 *	Set to \b true if case should be ignored when doing subsequent searches.
 * \param bExtended
 *	If \b true the POSIX Extended Regular Expression Syntax will be
 *	used. If \b false the POSIX Basic Regular Expression Syntax is
 *	used.
 * \return
 *	On success 0 is returned. On failure one of the ERR_* values described
 *	above is returned. All the error codes have negative values.
 * \sa Search(), Match()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t RegExp::Compile( const String & cExpression, bool bNoCase, bool bExtended )
{
	if( m->m_bValid )
	{
		regfree( &m->m_sRegex );
		m->m_cSubStrings.clear();
		delete[]m->m_pasMatches;
	}
	int nFlags = 0;

	if( bNoCase )
	{
		nFlags |= REG_ICASE;
	}
	if( bExtended )
	{
		nFlags |= REG_EXTENDED;
	}
	int nError = regcomp( &m->m_sRegex, cExpression.c_str(), nFlags );

	if( nError == 0 )
	{
		try
		{
			m->m_pasMatches = new regmatch_t[m->m_sRegex.re_nsub + 1];
			m->m_bValid = true;
			return ( 0 );
		}
		catch( ... )
		{
			m->m_pasMatches = NULL;
			m->m_bValid = false;
			return ( ERR_NOMEM );
		}
	}
	else
	{
		m->m_pasMatches = NULL;
		m->m_bValid = false;
		return ( conv_error( nError ) );
	}
}

/** Get the number of sub-expressions found in the previously compiled expression.
 * \par Description:
 *	Get the number of sub-expressions found in the previously compiled expression.
 *	If no expression have yet been compiled -1 is returned.
 * \return The number of sub-extressions found in the previously compiled
 *	   expression or -1 if no expression have yet been compiled.
 * \sa Compile()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int RegExp::GetSubExprCount() const
{
	if( m->m_bValid )
	{
		return ( m->m_sRegex.re_nsub );
	}
	else
	{
		return ( -1 );
	}
}

/** Check if a valid expression has been compiled.
 * \par Description:
 *	Returns \b true if the os::RegExp object represent a valid regular
 *	expression (last call to Compile() was successfull). Returns
 *	\b false if the default constuctor was used and no calls to Compile()
 *	have yet been made or if the last call to Compile() failed.
 * \sa Compile(), Search(), Match()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool RegExp::IsValid() const
{
	return ( m->m_bValid );
}

/** Search for the previously compiled regular expression.
 * \par Description:
 *	Search() will search for the last compiled regular expression in
 *	\p cString. If the pattern was found true is returned and the
 *	start and end position of the expression within \p cString is
 *	recorded. If the regular expression contain subexpression
 *	the sub-strings will be extracted and made availabel through
 *	GetSubString() and GetSubStrList().
 * \param cString
 *	The string to search.
 * \return Returns \b true if the pattern was found or false if no
 *	   match was found or if the os::RegExp object don't contain a
 *	   valid regular expression.
 * \sa Search(const String&,int,int), Match()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool RegExp::Search( const String & cString )
{
	if( m->m_bValid == false )
	{
		return ( false );
	}

	m->m_nStartPos = 0;
	m->m_cSubStrings.clear();
	bool bResult = regexec( &m->m_sRegex, cString.c_str(), m->m_sRegex.re_nsub + 1, m->m_pasMatches, 0 ) == 0;

	for( uint i = 1; i < m->m_sRegex.re_nsub + 1; ++i )
	{
		if( m->m_pasMatches[i].rm_so != -1 )
		{
			m->m_cSubStrings.push_back( String( cString.begin() + m->m_pasMatches[i].rm_so, cString.begin(  ) + m->m_pasMatches[i].rm_eo ) );
		}
		else
		{
			m->m_cSubStrings.push_back( String() );
		}
	}
	return ( bResult );
}

/** Search for the previously compiled regular expression.
 * \par Description:
 *	Same as Search(const String&) except that you can specify
 *	a range within the string that should be searched.
 * \param cString
 *	The string to search.
 * \param nStart
 *	Where in \p cString to start the search.
 * \param nLen
 *	How many characters from \p cString to search.
 * \return Returns \b true if the pattern was found or false if no
 *	   match was found or if the os::RegExp object don't contain a
 *	   valid regular expression.
 * \sa Search(const String&)
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool RegExp::Search( const String & cString, int nStart, int nLen )
{
	if( nLen == -1 )
	{
		nLen = String::npos;
	}
	bool bResult = Search( String( cString, nStart, nLen ) );

	m->m_nStartPos = nStart;
	return ( bResult );
}

/** Compare the regular expression to a string.
 * \par Description:
 *	Same as Search(const String&) except that the expression must
 *	match the entire string to be successfull.
 * \param cString
 *	The string that should be compared to the regular expression.
 * \return Returns \b true if the pattern matched or false if no
 *	   match was found or if the os::RegExp object don't contain a
 *	   valid regular expression.
 * \par Error codes:
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool RegExp::Match( const String & cString )
{
	if( m->m_bValid == false )
	{
		return ( false );
	}
	m->m_cSubStrings.clear();
	m->m_nStartPos = 0;
	bool bResult = regexec( &m->m_sRegex, cString.c_str(), m->m_sRegex.re_nsub + 1, m->m_pasMatches, 0 ) == 0;

	if( bResult == false || m->m_pasMatches[0].rm_so != 0 || m->m_pasMatches[0].rm_eo != int ( cString.size() ) )
	{
		return ( false );
	}
	for( uint i = 1; i < m->m_sRegex.re_nsub + 1; ++i )
	{
		if( m->m_pasMatches[i].rm_so != -1 )
		{
			m->m_cSubStrings.push_back( String( cString.begin() + m->m_pasMatches[i].rm_so, cString.begin(  ) + m->m_pasMatches[i].rm_eo ) );
		}
		else
		{
			m->m_cSubStrings.push_back( String() );
		}
	}
	return ( bResult );
}

/** Compare the regular expression to a string.
 * \par Description:
 *	Same as Search(const String&,int,int) except that the expression must
 *	match the entire sub-string to be successfull.
 * \param cString
 *	The string that should be compared to the regular expression.
 * \param nStart
 *	Where in \p cString to start the search.
 * \param nLen
 *	How many characters from \p cString to search.
 * \return Returns \b true if the pattern matched or false if no
 *	   match was found or if the os::RegExp object don't contain a
 *	   valid regular expression.
 * \sa Match(const string&), Search(const String&,int,int)
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool RegExp::Match( const String & cString, int nStart, int nLen )
{
	if( nLen == -1 )
	{
		nLen = String::npos;
	}
	bool bResult = Match( String( cString, nStart, nLen ) );

	m->m_nStartPos = nStart;
	return ( bResult );
}

/** Expand a string using substrings from the previous search.
 * \par Description:
 *	Expand a string using substrings from the previous search.
 *	Expand() will substitute "\%n" and "\%<nn>" constructs from
 *	\p cPattern with the string yielded by the referenced
 *	sub-expression and return the result in a new String object.
 * \par
 *	A substitution is initiated with a '\%' followed by a single
 *	digit or a single/multi-digit number within a <> construct.
 *	Sub expressions are numbered starting with 1.
 *	To insert sub-expression number 3 into a string you can
 *	use "\%3" or "\%<3> in the pattern. To insert sub-sexpression
 *	number 12 you must use "\%<12>". Two consecutive '\%' characters
 *	will insert one literal '\%' into the output.
 * \note
 *	In the current implementation a '\%' not followed by a digit or
 *	a <nn> construct and substitutions that reference out-of-range
 *	sub expressions will be inserted unmodified into the output.
 *	You should avoid such constructs though and always use "\%\%" to
 *	insert a literal '\%' into the output and keep the sub-expressions
 *	within range.
 * \param cPattern
 *	The pattern that should be expanded.
 * \return
 *	A copy of \p cPattern with all "\%n" and "\%<nn>" constructs
 *	expanded.
 * \sa Compile(), Search()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

String RegExp::Expand( const String & cPattern ) const
{
	String cBuffer;
	uint nStart = 0;
	uint i;

	for( i = 0; i < cPattern.size() - 1; ++i )
	{
		if( cPattern[i] == '%' )
		{
			if( cPattern[i + 1] == '%' )
			{
				cBuffer.str().insert( cBuffer.end(), cPattern.begin(  ) + nStart, cPattern.begin(  ) + i );
				nStart = i + 1;
				i++;
			}
			else
			{
				uint nIndex = 0;
				uint nLen = 0;

				if( cPattern[i + 1] == '<' )
				{
					nLen++;
				}
				while( i + 1 + nLen < cPattern.size() && isdigit( cPattern[i + 1 + nLen] ) )
				{
					nIndex *= 10;
					nIndex += cPattern[i + ( ++nLen )] - '0';
				}
				if( cPattern[i + 1] == '<' )
				{
					if( i + 1 + nLen >= cPattern.size() || cPattern[i + 1 + nLen] != '>' )
					{
						nLen = 0;
					}
				}
				if( nLen > 0 && nIndex > 0 && nIndex <= m->m_cSubStrings.size() )
				{
					cBuffer.str().insert( cBuffer.end(), cPattern.begin(  ) + nStart, cPattern.begin(  ) + i );
					cBuffer += m->m_cSubStrings[nIndex - 1];
					i += nLen;
					nStart = i + nLen;
				}
			}
		}
	}
	if( nStart < cPattern.size() )
	{
		cBuffer.str().insert( cBuffer.end(), cPattern.begin(  ) + nStart, cPattern.end(  ) );
	}
	return ( cBuffer );
}

/** Get the position of the first character that matched in the previous search.
 * \par Description:
 *	Get the position of the first character that matched in the previous search.
 *	This is the 0 based index of the first character from the searched string
 *	that matched the current regular expression. If the version of Search()
 *	that allow a range to be specified the start position will still be
 *	counted from the start of the string and not the start of the specified
 *	range.
 * \par
 *	If the last search was done with Match() the value will always be
 *	0 or the start position of the sub-string to match if a range was
 *	specified.
 * \par
 *	If no previous search has been performed or if the previous search failed
 *	this function will return -1.
 * \return The index of the first character in the searched string that
 *	   matched the regular expression during the previous search
 *	   or -1 if the previous search failed.
 * \par Error codes:
 * \sa GetEnd(), GetSubString(), Search(), Match()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int RegExp::GetStart() const
{
	if( m->m_bValid )
	{
		return ( m->m_pasMatches[0].rm_so );
	}
	else
	{
		return ( -1 );
	}
}

/** Get the position of the first character after the matched region in the previous search.
 * \par Description:
 *	Get the position of the first character after the matched region in the previous search.
 *	The rules for range-searces and failed/invalid searched that was described for
 *	GetStart() also apply for GetEnd().
 * \return The index of the first character after the match in the searched
 *	   string that matched the regular expression during the previous
 *	   search or -1 if the previous search failed.
 * \sa GetStart(), GetSubString(), Search(), Match()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

int RegExp::GetEnd() const
{
	if( m->m_bValid )
	{
		return ( m->m_pasMatches[0].rm_eo );
	}
	else
	{
		return ( -1 );
	}
}

/** Get the result of a subexpression from the previous search.
 * \par Description:
 *	Return the sub-string that matched sub-expression number
 *	\p nIndex. If the sub-expression was not used or if the
 *	index was out of range an empty string is returned.
 * \param nIndex
 *	A zero based index into the sub-expression list.
 * \return The result of the given sub-expression.
 * \sa GetSubString(uint,int*,int*), Search(), Match(), GetStart(), GetEnd()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

const String & RegExp::GetSubString( uint nIndex ) const
{
	static String cEmpty;

	if( nIndex >= m->m_cSubStrings.size() )
	{
		return ( cEmpty );
	}
	return ( m->m_cSubStrings[nIndex] );
}

/** Get the result of a subexpression from the previous search.
 * \par Description:
 *	This is the same as GetSubString(uint) except that it returns
 *	the range in the searched string instead of the actual sub-string
 *	itself.
 * \par
 *	If the specified sub-expression was not used or if \p nIndex
 *	is out of range \p pnStart and \p pnEnd will be set to -1.
 * \note
 *	If you specified a range for the previous search the positions
 *	will still be within the full searched string.
 * \param nIndex
 *	A zero based index into the sub-expression list.
 * \param pnStart
 *	Pointer to an integer that will receive the index of the first
 *	character that matched the specified sub-expression. If you
 *	are not interrested in the start-position NULL can be passed.
 * \param pnEnd
 *	Pointer to an integer that will receive the index of the first
 *	character after the region that matched the specified sub-expression.
 *	If you are not interrested in the end-position NULL can be passed.
 * \return Returns \b true if \p nIndex is within range and \b false if not.
 * \sa GetSubString(uint), Search(), Match(), GetStart(), GetEnd()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool RegExp::GetSubString( uint nIndex, int *pnStart, int *pnEnd ) const
{
	if( nIndex >= m->m_cSubStrings.size() || m->m_pasMatches[nIndex + 1].rm_so == -1 )
	{
		if( pnStart != NULL )
		{
			*pnStart = -1;
		}
		if( pnEnd != NULL )
		{
			*pnEnd = -1;
		}
		return ( nIndex < m->m_cSubStrings.size() );
	}
	else
	{
		if( pnStart != NULL )
		{
			*pnStart = m->m_pasMatches[nIndex + 1].rm_so + m->m_nStartPos;
		}
		if( pnEnd != NULL )
		{
			*pnEnd = m->m_pasMatches[nIndex + 1].rm_eo + m->m_nStartPos;
		}
		return ( true );
	}
}

/** Get a list of substrings from the previous search.
 * \par Description:
 *	Get a list of substrings from the previous search.
 *	If the previous search failed or if no search have
 *	been performed yet an empty list will be returned.
 * \return A STL vector with STL strings containing the
 *	   sub-strings from the previous search.
 * \sa GetSubString()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

const std::vector < String > &RegExp::GetSubStrList() const
{
	return ( m->m_cSubStrings );
}
