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

#include "file.h"
#include "commonfuncs.h"

/******************************************
* Description: Sourcery file is a container holding 
*        	   all the information on a file
* Author:      Rick Caudill
* Date:        Thu Mar 18 23:12:31 2004
*****************************************/
SourceryFile::SourceryFile(const os::String& cString)
{
	cFileName = cString;
	_Init();
}

/*****************************************
* Description: Destructor(makes sure that file handles are deleted)
* Author: Rick Caudill
* Date: Thu Mar 18 23:13:37 2004
*****************************************/
SourceryFile::~SourceryFile()
{
	if (pcFile) delete pcFile;
	if (pcNode) delete pcNode;
}

/*****************************************
* Description: Initializes all the file information
* Author: Rick Caudill
* Date: Thu Mar 18 23:14:35 2004
*****************************************/
void SourceryFile::_Init()
{
	try
	{
		pcNode = new os::FSNode(cFileName);
		pcFile = new os::File(*pcNode);
	}
	catch(...)
	{
		bValid = false;
		return;
	}
	bFirst = true;
	bValid = true;	
	cRelativePath = GetRelativePath(cFileName.c_str());
	cExtension = GetExtension(cFileName.c_str());
	
	if (pcNode->ReadAttr("Sourcery::CursorWidth",ATTR_TYPE_INT32,&nWidth,0,sizeof(int32)) <= 0)
	{
		nWidth = 0;
	}

	if (pcNode->ReadAttr("Sourcery::CursorHeight",ATTR_TYPE_INT32,&nHeight,0,sizeof(int32)) <=0)
	{
		nHeight = 0;
	}
	nSize = pcFile->GetSize();
	
	//read the lines from the file
	char zLines[nSize];
	pcFile->Read(zLines,nSize);
	zLines[nSize] = 0;
	cLines = os::String(zLines);
}

/*****************************************
* Description: Gets the lines of a file
* Author: Rick Caudill
* Date: Thu Mar 18 23:15:30 2004
*****************************************/
os::String SourceryFile::GetLines()
{
	return cLines;
}

/****************************************
* Description: Gets the extension of the file
* Author: Rick Caudill
* Date: Thu Mar 18 23:16:21 2004
*****************************************/
os::String SourceryFile::GetFilesExtension() 
{
	return cExtension;
}

/****************************************
* Description: Gets the files name
* Author: Rick Caudill
* Date: Thu Mar 18 23:17:17 2004
****************************************/
os::String SourceryFile::GetFileName() 
{
	return cFileName;
}

/****************************************
* Description: Gets the files relative path
* Author: Rick Caudill
* Date: Thu Mar 18 23:18:40 2004
****************************************/
os::String SourceryFile::GetFilesRelativePath()
{
	return cRelativePath;
}

/****************************************
* Description: Gets the size of a file
* Author: Rick Caudill
* Date: Thu Mar 18 23:20:03 2004
****************************************/	
int	SourceryFile::GetSize()
{
	return nSize;
}

/****************************************
* Description: Tells whether or not the file is a real file
* Author: Rick Caudill
* Date: Thu Mar 18 23:23:51 2004
****************************************/
bool SourceryFile::IsValid() 
{
	return bValid;
}

/****************************************
* Description: Gets the Cursor width attribute
* Author: Rick Caudill
* Date: Thu Mar 18 23:24:58 2004
****************************************/	
int	SourceryFile::GetCursorWidth() 
{
	return nWidth;
}

/****************************************
* Description: Gets the cursor height attribute
* Author: Rick Caudill
* Date: Thu Mar 18 23:25:42 2004
****************************************/
int SourceryFile::GetCursorHeight() 
{
	return nHeight;
}

/****************************************
* Description: Deletes the file handles and sets the class to invalid state
* Author: Rick Caudill
* Date: Thu Mar 18 23:26:32 2004
****************************************/
void SourceryFile::DeleteFiles()
{
	delete pcFile;
	delete pcNode;
	bValid = false;
}

/****************************************
* Description: Reinitialize the whole class(not really the way to do it)
* Author: Rick Caudill
* Date: Thu Mar 18 23:27:37 2004
****************************************/
void SourceryFile::ReInitialize()
{
	if (pcFile)
		delete pcFile;
	if (pcNode)
		delete pcNode;
	
	try
	{
		pcNode = new os::FSNode(cFileName);
		pcFile = new os::File(*pcNode);
	}
	catch(...)
	{
		bValid = false;
		return;
	}

	bValid = true;
	bFirst = true;
	cLines = "";	
	cRelativePath = GetRelativePath(cFileName.c_str());
	cExtension = GetExtension(cFileName.c_str());
	pcNode->ReadAttr("Sourcery::CursorWidth",ATTR_TYPE_INT32,&nWidth,0,sizeof(int32));
	pcNode->ReadAttr("Sourcery::CursorHeight",ATTR_TYPE_INT32,&nHeight,0,sizeof(int32));	
	nSize = pcFile->GetSize();

	char zLines[nSize];
	pcFile->Read(zLines,nSize);
	zLines[nSize] = 0;
	cLines = os::String(zLines);	
}

/************************************
* Description: Gets the created time from the node
* Author: Rick Caudill
* Date: Thu Mar 18 23:28:41 2004
************************************/
DateTime* SourceryFile::GetDateTime()
{
	if (pcNode)
	{
		DateTime* pcReturn = new DateTime(pcNode->GetCTime(true));
		return pcReturn;
	}
	else
	 	return NULL;
}



