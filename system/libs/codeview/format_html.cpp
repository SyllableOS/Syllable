/*	libcodeview.so - A programmers editor widget for Atheos
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
#include "format_html.h"

#include <util/string.h>

using namespace std;
using namespace cv;

//contexts
#define C_NONE          0x00000000

#define C_COMMENT   		0x00000001
#define C_TAG		   	0x01000000
#define C_COMMENT_OPEN_2	0x02000000
#define C_COMMENT_OPEN_3	0x04000000
#define C_COMMENT_CLOSE_1   	0x10000000
#define C_COMMENT_CLOSE_2	0x20000000

static const os::String names[]={
	"Default",
	"Comment",
	"Tag",
	"Keyword",
	"Attribute"
};

static const os::String unusedname="<Not Used>";

static const os::Color32_s defaultcolors[]={
	os::Color32_s(  0,   0,   0), // default, black
	os::Color32_s(  0, 128,   0), // comment, dark green
	os::Color32_s(128,   0,   0), // tag default, dark red
	os::Color32_s(  0,   0, 255), // tag keyword, blue
	os::Color32_s(255,   0,   0), // tag attribute, red
};


// Tagname list. Keep these sorted!
static const char *tagnames[] = {	// Strict DTD
	"a",
	"abbr",
	"acronym",
	"address",
	"area",
	"b",
	"base",
	"bdo",
	"big",
	"blockquote",
	"body",
	"br",
	"button",
	"caption"
	"cite",
	"code",
	"col",
	"colgroup",
	"dd",
	"del",
	"dfn",
	"div",
	"dl",
	"dt",
	"em",
	"fieldset",
	"form",
	"h1",
	"h2",
	"h3",
	"h4",
	"h5",
	"h6",
	"head",
	"hr",
	"html",
	"i",
	"img",
	"input",
	"ins",
	"kbd",
	"label",
	"legend",
	"li",
	"link",
	"map",
	"meta",
	"noscript",
	"object",
	"ol",
	"optgroup",
	"option",
	"p",
	"param",
	"pre",
	"q",
	"samp",
	"script",
	"select",
	"small",
	"span",
	"strong",
	"style",
	"sub",
	"sup",
	"table",
	"tbody",
	"td",
	"textarea",
	"tfoot",
	"th",
	"thead",
	"title",
	"tr",
	"tt",
	"ul",
	"var"
};

// Tag attribute list. Keep these sorted!
static const char *attribs[] = {
	"align=",
	"background=",
	"bgcolor=",
	"border=",
	"bordercolor=",
	"cellpadding=",
	"cellspacing=",
	"face=",
	"height=",
	"href=",
	"link=",
	"size=",
	"src=",
	"text=",
	"type=",
	"vlink=",
	"width="
};

Format_HTML::Format_HTML()
{
    for( int a = 0; a < FORMAT_COUNT; a++ )
	styles[ a ].sColor = defaultcolors[ a ];
}

uint Format_HTML::GetStyleCount()
{
    return FORMAT_COUNT;
}

const os::String & Format_HTML::GetStyleName( char nType )
{
    if( nType < 0 || nType >= FORMAT_COUNT )
	return ::unusedname;

    return ::names[ nType ];
}

const CodeViewStyle& Format_HTML::GetStyle( char nType )
{
    if( nType < 0 || nType >= FORMAT_COUNT )
	nType = 0;

    return styles[nType];
}

void Format_HTML::SetStyle( char nType, const CodeViewStyle& cStyle )
{
    if( nType < 0 || nType >= FORMAT_COUNT )
	return;

    styles[ nType ] = cStyle;
}

static inline bool canStartIdentifier(char c) { return (c>='a' && c<='z') || (c>='A' && c<='Z'); }
static inline bool canContinueIdentifier(char c) { return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='=' || c=='/'; }

void Format_HTML::FindWords(const os::String &line, os::String &format)
{
	uint start=0, end;
	const char **keywords;
	char fmt = F_KEYWORD;
	int max;

	do {
		// 1. Find the start an identifier inside a tag
		while( start<line.size() && line[ start ] != '<' && (format[start]!=F_TAG || !canStartIdentifier(line[start])) )
			++start;

		if( start >= line.size() ) return;

		// 2. Find the end of the identifier.		
		end = start + 1;
		while( end<line.size() && format[end]==F_TAG && canContinueIdentifier(line[end]) ) {
			if( line[ end++ ] == '=' ) break;
		}
	
		os::String tmp;
	
		// 3. Adjust starting position and find out whether it's a tag name or an attribute
		if( line[ start ] == '<' ) {
			start++;
			if( line[ start ] == '/' ) {
				tmp = line.substr( start + 1, end - start - 1 );
			} else {
				tmp = line.substr( start, end - start );
			}
			keywords = tagnames;
			max = sizeof( tagnames ) / sizeof( tagnames[ 0 ] );
			fmt = F_KEYWORD;
		} else {
			keywords = attribs;
			max = sizeof( attribs ) / sizeof( attribs[ 0 ] );
			fmt = F_ATTRIBUTE;
			tmp = line.substr(start, end-start);
		}

		// 4. Search the keywords array for the identifier
		int min=0;
		int test=max/2;

		while( max - min > 1 ) {
			if( tmp.CompareNoCase( keywords[ test ] ) == 0 ) {
				for( ; start < end; ++start )
					format[ start ] = fmt;
				break;
			}

			if( tmp.CompareNoCase( keywords[ test ] ) < 0 )
				max = test;
			else
				min = test;
			test = ( max + min ) / 2;
		}

		start = end + 1;

	} while( start<line.size() );
}

CodeViewContext Format_HTML::Parse( const os::String &line, os::String &format, CodeViewContext cookie )
{
    int oldcntx = cookie.nContext, newcntx=cookie.nContext;
    char c;

    format.resize( line.size() );

    for( uint a = 0; a < line.size(); a++ ) {
	newcntx = oldcntx & ( C_COMMENT | C_TAG );

	switch( line[ a ] ) {
	    case '<':
	    	if( line.substr( a, 4 ) == "<!--" ) {
	    		newcntx |= C_COMMENT;
	    	} else {
			newcntx |= C_TAG;
			
		}
		break;
	    case '>':
		if((oldcntx & C_TAG) && !(oldcntx & C_COMMENT))
			newcntx &= ~C_TAG;
	    	break;
	}

	if(newcntx & C_COMMENT)
		format[a] = F_COMMENT;
	else if(newcntx & C_TAG)
		format[a] = F_TAG;
	else
		format[a] = F_DEFAULT;

	if((oldcntx & C_TAG) && !(newcntx & C_TAG))
	    format[a] = F_TAG;

	if( line[ a ] == '-' ) {
    	    if( oldcntx & C_COMMENT ) {
    	        if( line.substr( a, 3 ) == "-->" ) {
    	    	    format[a] = F_COMMENT;
    	    	    format[a+1] = F_COMMENT;
    	    	    format[a+2] = F_COMMENT;
    	    	    newcntx &= ~C_COMMENT;
    	        }
    	    }
	}

	oldcntx = newcntx;
    }

    // Scan line for tag keywords and tag attributes:
    FindWords( line, format );

    return newcntx & C_COMMENT;
}

os::String Format_HTML::GetIndentString(const os::String &line, bool useTabs, uint tabSize){
	if(line.size()==0)
		return "";

	uint white;
	for(white=0;white<line.size();++white)
		if(line[white]!=' ' && line[white]!='\t')
			break;
			
	return line.substr(0, white);
}

uint Format_HTML::GetPreviousWordLimit(const os::String &line, uint chr){
	if(chr==0)
		return 0;
	--chr;

	if(canContinueIdentifier(line[chr])){//in identifier
		while(chr>0 && canContinueIdentifier(line[chr]))
			--chr;
		if(!canContinueIdentifier(line[chr]))
			++chr;
	}else{//outside identifier
		while(chr>0 && !canContinueIdentifier(line[chr]))
			--chr;
		if(canContinueIdentifier(line[chr]))
			++chr;
	}
	return chr;
}

uint Format_HTML::GetNextWordLimit(const os::String &line, uint chr){
	uint max=line.size();
	if(chr>=max)
		return max;

	if(canContinueIdentifier(line[chr])){//in identifier
		while(chr<=max && canContinueIdentifier(line[chr]))
			++chr;
	}else{//outside identifier
		while(chr<=max && !canContinueIdentifier(line[chr]))
			++chr;
	}
	return chr;
}
