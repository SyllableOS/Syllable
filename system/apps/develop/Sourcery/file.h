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


#ifndef _SOURCERY_FILE_H
#define	_SOURCERY_FILE_H

#include <util/string.h>
#include <util/datetime.h>
#include <storage/file.h>
#include <storage/fsnode.h>
#include <iostream>

using namespace std;
using namespace os;

class SourceryFile
{
public:
	SourceryFile(const os::String& cString);
	~SourceryFile();	
	
	int 		GetCursorHeight(); 	
	int			GetCursorWidth();
	bool 		IsValid();
	int 		GetSize();
	void		DeleteFiles();
	void		ReInitialize();

	os::String 	GetFilesRelativePath();
	os::String 	GetFileName(); 
	os::String 	GetFilesExtension(); 
	os::String 	GetLines();
	os::DateTime* GetDateTime();  
private:		
	
	void _Init();


	os::File* pcFile;
	os::FSNode* pcNode;
	os::DateTime* pcDate;

	bool bValid;
	bool bFirst;
	
	os::String cFileName;
	os::String cExtension;
	os::String cRelativePath;
	os::String cLines;

	int	nBytes;
	int nSize;
	int nWidth;
	int nHeight;
};

#endif  //_SOURCERY_FILE_H
