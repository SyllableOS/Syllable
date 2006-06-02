//  Sourcery -:-  (C)opyright 2003-2004 Rick Caudill
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

#ifndef _COMMON_FUNCS_H_
#define _COMMON_FUNCS_H_

//libsyllable
#include <util/string.h>
#include <util/datetime.h>
#include <gui/image.h>
#include <util/resources.h>
#include <atheos/image.h>

//codeview specific
#include <codeview/format.h>
#include <codeview/format_c.h>
#include <codeview/format_java.h>
#include <codeview/format_perl.h>
#include <codeview/format_html.h>

//standard
#include <unistd.h>
#include <pwd.h>
#include <vector>

//mine
#include "formatset.h"
#include "file.h"

typedef std::vector<FormatSet> tList;

using namespace os;

String 				GetDate();
String				GetDateAndTime();
String				GetTime();
String 				GetHeader();
String				GetComment(const String& cFileType);
String				GetRelativePath(const String& cPath);
String				GetExtension(const String& cPath);
String				ConvertSizeToString(uint);
String				ConvertIntToString(int);
String				ConvertCrLfToLf(const String&);
String				RemoveExtension(const String&);
String				GetDirectoryofFileOpened(const String& cFile);
String				GetFileNameOnly(const String&);
String				ParseForMacros(const String&, SourceryFile*);
String				ParseForUnderScores(const String&);
String				GetSystemInfo();
String				GetBuildInfo();
String				GetName();
String				GetLinesFromFile(File*);
String				GetApplicationPath();

int					GetCommentCount(const String&);
int					GetLineCount(const String&);

tList				GetFormats();

void				SetBackupFile(const String&, const String&);

os::BitmapImage* 	LoadImageFromResource( const os::String &cName );
Color32_s 			Tint( const Color32_s & sColor, float vTint );

#endif  //_COMMON_FUNCS_H_
