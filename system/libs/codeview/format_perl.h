/*	libcodeview.so - A programmers editor widget for Syllable
	Copyright (c) 2001 Andreas Engh-Halstvedt
	Copyright (c) 2003 Henrik Isaksson

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
	MA 02111-1307, USA
*/

#ifndef F_CODEVIEW_FORMAT_PERL_H
#define F_CODEVIEW_FORMAT_PERL_H

#include "format.h"

namespace cv
{

/**An implementation of a formater for Perl
 * \author Henrik Isaksson (henrik@boing.nu)
 */ 
class Format_Perl : public Format
{
    public:
    Format_Perl();

    uint GetStyleCount();
    const os::String& GetStyleName( char );
    void SetStyle( char, const CodeViewStyle& );
    const CodeViewStyle& GetStyle( char );

    CodeViewContext Parse( const os::String &cLine, os::String &cFormat, CodeViewContext cookie );

    os::String GetIndentString( const os::String &cText, bool bUseTabs, uint nTabSize );

    uint GetPreviousWordLimit( const os::String&, uint nChr );
    uint GetNextWordLimit( const os::String&, uint nChr );

    private:
    enum {
	F_DEFAULT = 0,
	F_COMMENT,
	F_STRING,
	F_KEYWORD,

	FORMAT_COUNT//this is not a format!
    };

    CodeViewStyle styles[FORMAT_COUNT];

    void FindWords(const os::String&, os::String&);
};

} /* namespace cv */

#endif /* F_CODEVIEW_FORMAT_PERL_H */
