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

#ifndef _TOOL_HANDLE_H
#define _TOOL_HANDLE_H

struct ToolHandle
{
	char zName[1024];
	char zCommand[1024];
	char zArguments[1024];
	bool bReload;  //next version
};

#endif  //_TOOL_HANDLE_H
