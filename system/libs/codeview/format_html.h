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

#ifndef F_CODEVIEW_FORMAT_HTML_H
#define F_CODEVIEW_FORMAT_HTML_H

#include "format.h"

namespace cv
{

/**An implementation of a formater for HTML
 * \author Henrik Isaksson (henrik@boing.nu)
 */ 
class Format_HTML : public Format
{
    public:
    Format_HTML();

    uint GetStyleCount();
    const string& GetStyleName( char );
    void SetStyle( char, const CodeViewStyle& );
    const CodeViewStyle& GetStyle( char );

    CodeViewContext Parse( const std::string &cLine, std::string &cFormat, CodeViewContext cookie );

    std::string GetIndentString( const std::string &cText, bool bUseTabs, uint nTabSize );

    uint GetPreviousWordLimit( const std::string&, uint nChr );
    uint GetNextWordLimit( const std::string&, uint nChr );

    private:
    enum {
	F_DEFAULT = 0,
	F_COMMENT,
	F_TAG,
	F_KEYWORD,
	F_ATTRIBUTE,

	FORMAT_COUNT//this is not a format!
    };

    CodeViewStyle styles[FORMAT_COUNT];

    void FindWords(const string&, string&);
};

} /* namespace cv */

#endif /* F_CODEVIEW_FORMAT_HTML_H */


