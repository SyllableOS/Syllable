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

#ifndef _FORMAT_SET_H
#define _FORMAT_SET_H

#include <util/string.h>
#include <codeview/format.h>


class FormatSet
{
public:
	FormatSet();
	~FormatSet();

    std::string	cName;
    std::string cFilter;
    std::string	cFormatName;
    cv::Format* pcFormat;

    os::Color32_s sFgColor;
    os::Color32_s sBgColor;

    int32 nTabSize;
    bool bUseTab;
    bool bAutoIndent;
};

#endif  //_FORMAT_SET_H
