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

#ifndef __F_UTIL_OPTIONPARSER_H__
#define __F_UTIL_OPTIONPARSER_H__

#include <stdio.h>
#include <atheos/types.h>
#include <util/string.h>
#include <storage/streamableio.h>

#include <vector>

namespace os {
#if 0
} // Fool Emacs autoindent
#endif


/** Command line option parser.
 * \ingroup util
 * \par Description:
 *	os::OptionParser is a utility class that can help you parse command
 *	line options. It also have members for automatic generation of a
 *	help text suitable for the "--help" option available in most command
 *	line commands. The help text feature will format the text according
 *	to the current terminal widht (or optionally a suplied with) performing
 *	necessarry word-wrapping to make the text as readable as possible.
 * \par
 *	The rules for option parsing is mostly compatible with the GNU getopt()
 *	function. There is two kind of options. Short-options or flag start with
 *	a single "-" and ends with a single letter. Long options start with
 *	"--" and end with a string. Both short-options and long-options can
 *	have additional arguments. If a short-option take an additional argument
 *	it the next option will be used as the argument value. If a long-option
 *	take an additional argument it must be specified as "--opt=arg".
 *	All arguments that are not recognized as options will be added to a
 *	"file-list" that can later be iterated. If one of the arguments is
 *	"--" the option parsing will stop there and the rest of the arguments
 *	will be added unconditionally to the "file-list".
 * \since 0.3.7
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class OptionParser
{
public:
    struct option {
		char 	    opt;
		String long_name;
		String raw_opt;
		String value;
		bool	    has_value;
    };
    struct argmap {
		const char* long_name;
		char	    opt;
		const char* desc;
    };

    OptionParser( int argc, const char * const * argv );
    ~OptionParser();

    void AddArgMap( const argmap* pasMap );
    void AddArgMap( const String& cLongArg, char nShortArg, const String& cDesc );
    void ParseOptions( const char* pzOptions );

    int  GetOptionCount() const;
    int	 GetFileCount() const;
    
    const option* FindOption( char nOpt ) const;
    const option* FindOption( const String& cLongName ) const;
    
    const option* GetNextOption();
    void	  RewindOptions();
    const option* GetOption( uint nIndex ) const;
    
    
    const std::vector<String>& GetArgs() const;
    const std::vector<String>& GetFileArgs() const;

    String GetHelpText( int nWidth = 0 ) const;
    void	PrintHelpText( int nWidth = 0 ) const;
	void	PrintHelpText( StreamableIO* pcStream, int nWidth = 0 ) const;
    
    String operator[](int nIndex) const;
private:
    char _MapLongOpt( const String& cOpt ) const;
    class Private;
    Private* m;
};


} // end of namespace

#endif // __F_UTIL_OPTIONPARSER_H__
