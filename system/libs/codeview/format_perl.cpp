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
#include "format_perl.h"

using namespace std;
using namespace cv;

//contexts
#define C_NONE          0x00000000

#define C_LINECOMMENT   0x00000001
#define C_SPANCOMMENT   0x00000002

#define C_CHARCONST     0x00000010
#define C_STRINGCONST   0x00000020

#define C_ESCAPE        0x00001000
#define C_SLASH         0x00002000
#define C_STAR          0x00004000

	//for convenience
#define C_COMMENT       (C_LINECOMMENT | C_SPANCOMMENT)
#define C_CONST         (C_STRINGCONST | C_CHARCONST)
#define C_FLAGS         (C_ESCAPE | C_SLASH | C_STAR)


static const os::String names[]={
	"Default",
	"Comment",
	"String",
	"Keyword"
};

static const os::String unusedname="<Not Used>";

static const os::Color32_s defaultcolors[]={
	os::Color32_s(  0,   0,   0),//default, black
	os::Color32_s(  0, 128,   0),//comment, dark green
	os::Color32_s(128,   0,   0),//string, dark red
	os::Color32_s(255,   0,   0),//char, red;
	os::Color32_s(  0,   0, 255),//keyword, blue
};


//Keyword list. Keep these sorted!
#ifndef OVERRIDE_KEYWORDS
static const char *keywords[]={
	"accept",
	"alarm",
	"atan2",
	"bind",
	"binmode",
	"bless",
	"break",
	"caller",
	"case",
	"chdir",
	"chmod",
	"chomp",
	"chop",
	"chown",
	"chr",
	"chroot",
	"close",
	"closedir",
	"connect",
	"continue",
	"cos",
	"crypt",
	"dbmclose",
	"dbmopen",
	"default",
	"defined",
	"delete",
	"die",
	"do",
	"dump",
	
	"else",
	"elsif",
	"eq",
	"for",
	"foreach",
	"goto",
	"if",
	"join",
	"last",
	"local",
	"my",
	"ne",
	"new",
	"next",
	"no",
	"open",
	"or",
	"our",
	"package",
	"print",
	"push",
	"redo",
	"require",
	"return",
	"scalar",
	"split",
	"strict",
	"sub",
	"switch",
	"undef",
	"unless",
	"until",
	"use",
	"while"
};
#endif

Format_Perl::Format_Perl()
{
    for( int a = 0; a < FORMAT_COUNT; a++ )
	styles[ a ].sColor = defaultcolors[ a ];
}

uint Format_Perl::GetStyleCount()
{
    return FORMAT_COUNT;
}

const os::String & Format_Perl::GetStyleName( char nType )
{
    if( nType < 0 || nType >= FORMAT_COUNT )
	return ::unusedname;

    return ::names[ nType ];
}

const CodeViewStyle& Format_Perl::GetStyle( char nType )
{
    if( nType < 0 || nType >= FORMAT_COUNT )
	nType = 0;

    return styles[nType];
}

void Format_Perl::SetStyle( char nType, const CodeViewStyle& cStyle )
{
    if( nType < 0 || nType >= FORMAT_COUNT )
	return;

    styles[ nType ] = cStyle;
}


static inline bool canStartIdentifier(char c) { return c=='_' || (c>='a' && c<='z') || (c>='A' && c<='Z'); }
static inline bool canContinueIdentifier(char c) { return c=='_' || (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9'); }


//TODO: this method is slightly wrong. Fix it
void Format_Perl::FindWords(const os::String &line, os::String &format){
	uint start=0, end;

	while(start<line.size() && (format[start]!=F_DEFAULT || !canStartIdentifier(line[start])))
		++start;

	end=start+1;
	while(end<line.size() && format[end]==F_DEFAULT && canContinueIdentifier(line[end]))
		++end;

	while(start<line.size()){
		os::String tmp=line.substr(start, end-start);

		int max=sizeof(keywords)/sizeof(keywords[0]);
		int min=0;
		int test=max/2;

		//binary search
		while(max-min>1){
			if(os::String(keywords[test])==tmp)
				break;
			if(tmp<keywords[test])
				max=test;
			else
				min=test;
			test=(max+min)/2;
		}
		if(os::String(keywords[test])==tmp){
			for(;start<end;++start)
				format[start]=F_KEYWORD;
		}

		start=end+1;

		while(start<line.size() && (format[start]!=F_DEFAULT || !canStartIdentifier(line[start])))
			++start;

		end=start+1;
		while(end<line.size() && format[end]==F_DEFAULT && canContinueIdentifier(line[end]))
			++end;
	}
}

CodeViewContext Format_Perl::Parse(const os::String &line, os::String &format, CodeViewContext cookie){
	int oldcntx=cookie.nContext, newcntx=cookie.nContext;
	char c;

	format.resize(line.size());

	for(uint a=0;a<line.size();++a){
		newcntx = oldcntx & ( C_COMMENT | C_CONST );
		c = line[a];


		switch(line[a]){
		//if found '\'' ->
		//	if context is comment or string: ignore
		//	if context is charconst and escape: ignore
		//	if context is charconst and !escape: end charconst
		//	if context is !charconst and !escape: begin charconst
		case '\'':
			if(oldcntx & (C_COMMENT | C_STRINGCONST))
				break;

			if(oldcntx & C_CHARCONST){
				if(oldcntx & C_ESCAPE){
					break;
				}else{
					newcntx &= ~C_CHARCONST;
				}
			}else{
				if(!(oldcntx & C_ESCAPE)){
					newcntx |= C_CHARCONST;
				}
			}
			break;
		//if found '"' ->
		//	if context is comment or charconst: ignore
		//	if context is string and escape; ignore
		//	if context is string and !escape: end string
		//	if context is !string and !escape: begin string
		case '"':
			if(oldcntx & (C_COMMENT | C_CHARCONST))
				break;

			if(!(oldcntx & C_ESCAPE)){
				if(oldcntx & C_STRINGCONST){
					newcntx &= ~C_STRINGCONST;
				}else{
					newcntx |= C_STRINGCONST;
				}
			}
			break;
		//if found '\\' ->
		//	if context is !escape: set escape
		case '\\':
			if(!(oldcntx & C_ESCAPE))
				newcntx |= C_ESCAPE;
			break;
		case '#':
			if(!(oldcntx & C_ESCAPE))
				newcntx |= C_LINECOMMENT;
			break;
		//if found '/' ->
		//	if context is !slash: set slash
		case '/':	
			if( oldcntx & (C_COMMENT | C_CONST) )
				break;

			newcntx |= C_SLASH;
			break;
		}

		if(newcntx & C_CHARCONST)
			format[a] = F_STRING;
		else if(newcntx & C_STRINGCONST)
			format[a] = F_STRING;
		else if(newcntx & C_COMMENT)
			format[a] = F_COMMENT;
		else
			format[a] = F_DEFAULT;

		//special handling of closing quotes
		if((oldcntx & C_STRINGCONST) && !(newcntx & C_STRINGCONST))
			format[a] = F_STRING;
		else if((oldcntx & C_CHARCONST) && !(newcntx & C_CHARCONST))
			format[a] = F_STRING;

		oldcntx = newcntx;
	}

	//now look for words...
	FindWords(line, format);



	if(newcntx&C_ESCAPE)
		return newcntx & ( C_COMMENT | C_CONST );
	else
		return newcntx & ( C_SPANCOMMENT );
}

os::String Format_Perl::GetIndentString( const os::String &line, bool useTabs, uint tabSize )
{
	if(line.size()==0)
		return "";

	uint white;
	for(white=0;white<line.size();++white)
		if(line[white]!=' ' && line[white]!='\t')
			break;
			
	//TODO: ignore trailing whitespace
	if(line[line.size()-1]=='{' || line[line.size()-1]==':'){
		string pad;		
		if(useTabs)
			pad="\t";
		else
			pad.resize(tabSize, ' ');
			
		return line.substr(0, white)+pad;
	}else
		return line.substr(0, white);
}

uint Format_Perl::GetPreviousWordLimit(const os::String &line, uint chr)
{
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

uint Format_Perl::GetNextWordLimit(const os::String &line, uint chr)
{
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
