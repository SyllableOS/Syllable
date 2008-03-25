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

#include "commonfuncs.h"
#include <codeview/format_c.h>
#include <codeview/format_java.h>
#include <codeview/format_perl.h>
#include <codeview/format_html.h>
#include <codeview/format_ruby.h>

/*********************************************
* Description: Returns the current date as a string
* Author: Rick Caudill
* Date: Thu Mar 18 21:26:29 2004
**********************************************/
String GetDate()
{
	DateTime d = DateTime::Now();
	char bfr[256];
	struct tm *pstm;
	time_t nTime = d.GetEpoch();

	pstm = localtime( &nTime );

	strftime( bfr, sizeof( bfr ), "%a %b %e %Y", pstm );
	return String(bfr);
}

/*********************************************
* Description: Returns the current time as a string
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetTime()
{
	DateTime d = DateTime::Now();
	char bfr[256];
	struct tm *pstm;
	time_t nTime = d.GetEpoch();

	pstm = localtime( &nTime );

	strftime( bfr, sizeof( bfr ), "%T", pstm );
	return String(bfr);
}

/*********************************************
* Description: Returns the current time as a string
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetDateAndTime()
{
	DateTime d = DateTime::Now();
	return d.GetDate();
}	

/*********************************************
* Description: Returns a program header
* Author: Rick Caudill
* Date: Thu Mar 18 21:27:19 2004
**********************************************/
String GetHeader()
{
	String cReturn;
	cReturn = (String)"/************************************\n* Program:\n* Description:\n* Author:\n* Date: " + GetDate();
	cReturn += "\n************************************/";   
	return cReturn;
}

/*************************************************
* Description: Returns a comment string determined by the comment type
* Author: Rick Caudill
* Date: Thu Mar 18 21:28:16 2004
*************************************************/
String GetComment(const String& cFileType)
{
	String cReturn;
	
	if (cFileType == ".C" || cFileType == ".c")
	{
		cReturn = "/* */";
	}
	else if (cFileType == ".cpp" || cFileType == ".hpp" || cFileType == ".cxx" || cFileType == ".h"|| cFileType == ".cc")
		cReturn = "//";

	else if (cFileType == ".perl" || cFileType == ".pm" || cFileType == ".sh" || cFileType == ".rb" || cFileType == ".ruby")
		cReturn = "#";

	else if (cFileType == ".java")
		cReturn = "//";

	else if (cFileType == ".html" || cFileType == ".htm")
		cReturn = "<!-- -->";
 
	return cReturn;
}

/**********************************************
* Description: Gets the formats and returns them as a vector
* Author: Rick Caudill
* Date: Thu Mar 18 21:29:13 2004
***********************************************/
tList GetFormats()
{
	tList formatList;
	formatList.resize( 6 );
	formatList[0].cName = "Default";
	formatList[0].cFilter = "N/A";
	formatList[0].cFormatName = "Null formatter";
	formatList[1].cName = "C/C++";
	formatList[1].cFilter = ".c;.h;.C;.cpp;.hpp;.cxx;.cc";
	formatList[1].pcFormat = new cv::Format_C();
	formatList[1].cFormatName = "C formatter";
	formatList[2].cName = "Java";
	formatList[2].cFilter = ".java";
	formatList[2].pcFormat = new cv::Format_java();
	formatList[2].cFormatName = "Java formatter";
	formatList[3].cName = "Perl";
	formatList[3].cFilter = ".perl;.pm;";
	formatList[3].pcFormat = new cv::Format_Perl();
	formatList[3].cFormatName = "Perl formatter";
	formatList[4].cName = "HTML";
	formatList[4].cFilter = ".htm;.html;.HTM;.HTML";
	formatList[4].pcFormat = new cv::Format_HTML();
	formatList[4].cFormatName = "HTML formatter";
	formatList[5].cName = "Ruby";
	formatList[5].cFilter = ".rb;.ruby";
	formatList[5].pcFormat = new cv::Format_Ruby();
	formatList[5].cFormatName = "Ruby formatter";
	return formatList;
}

/****************************************************
* Description: Gets the relative path of a path passed
* Author: Rick Caudill
* Date: Thu Mar 18 21:30:04 2004
****************************************************/
os::String GetRelativePath(const String& cPath)
{
	std::string cRelativePath = cPath.c_str();
	int nLocation = cRelativePath.find_last_of("/");
	if (nLocation > -1)
		return (String(cRelativePath.substr(nLocation+1,cRelativePath.size()-nLocation)));
	else 
		return cPath;
}

/***************************************************
* Description: Finds the extension of the file(this will be superceded when mimetypes are done)
* Author: Rick Caudill
* Date: Thu Mar 18 21:31:04 2004
***************************************************/
os::String GetExtension(const String& cPath)
{
	std::string cExtension = cPath.c_str();
	int nLocation = cExtension.find_last_of(".");
	if (nLocation >-1)
		return (String(cExtension.substr(nLocation,cExtension.size()-nLocation)));
	else
		return cPath;
}

/************************************************
* Description: Loads a bitmapimage from resource
* Author: Rick Caudill
* Date: Thu Mar 18 21:32:37 2004
*************************************************/
os::BitmapImage* LoadImageFromResource( const os::String &cName )
{
	os::BitmapImage * pcImage = new os::BitmapImage();
	os::Resources cRes( get_image_id() );
	os::ResStream * pcStream = cRes.GetResourceStream( cName.c_str() );
	if( pcStream == NULL )
	{
		throw( os::errno_exception("") );
	}
	pcImage->Load( pcStream );
	return pcImage;
}

/************************************************
* Description: Darkens or lightens the color depending on the tint value passed.
* Author: Kurt Skauen
* Date: Thu Mar 18 21:33:11 2004
*************************************************/
Color32_s Tint( const Color32_s & sColor, float vTint )
{
	int r = int ( ( float ( sColor.red ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int g = int ( ( float ( sColor.green ) * vTint + 127.0f * ( 1.0f - vTint ) ) );
	int b = int ( ( float ( sColor.blue ) * vTint + 127.0f * ( 1.0f - vTint ) ) );

	if( r < 0 )
		r = 0;
	else if( r > 255 )
		r = 255;
	if( g < 0 )
		g = 0;
	else if( g > 255 )
		g = 255;
	if( b < 0 )
		b = 0;
	else if( b > 255 )
		b = 255;
	return ( Color32_s( r, g, b, sColor.alpha ) );
}

/*********************************************
* Description: Simple function that creates a backup file
* Author: Rick Caudill
* Date: Thu Mar 18 21:34:24 2004
*********************************************/
void SetBackupFile(const os::String& cBackup, const os::String& cExtension)
{
	String cBackupFile = String(getenv("HOME")) + String("/Settings/Sourcery/Backups/") + GetFileNameOnly(cBackup) + (String)cExtension.c_str();
	String cExecute = "cp -f ";
	cExecute += cBackup;
	cExecute += " ";
	cExecute +=  cBackupFile; 
	system(cExecute.c_str());
}

/********************************************
* Description: Converts the size of a file from bytes into the designated size
* Author: Rick Caudill
* Date: Thu Mar 18 21:35:08 2004
********************************************/
String ConvertSizeToString(uint nSize)
{
	char zReturn[255]="";
	if (nSize >= 1000000)  //mega bytes
		sprintf(zReturn,"%i Mb",nSize/1000000 +1);
	else if (nSize >= 1000) //kilo bytes
		sprintf(zReturn,"%i Kb",nSize/1000 +1);
	else //not big enough to be passed bytes
		sprintf(zReturn, "%i Bytes",nSize);
	return (String(zReturn));
}

/*********************************************
* Description: Parses a string to count how many lines there are
* Author: Rick Caudill
* Date: Thu Mar 18 21:37:28 2004
**********************************************/
int GetLineCount(const String& cLines)
{
	int nLineCount=0;
	//TODO: Probably could use find() instead of parsing every char	
	for (uint i=0; i<cLines.size(); ++i)
	{
		if (cLines[i] == '\n')
		{
			nLineCount++;
		}
	}
	return (nLineCount);				
}

/********************************************
* Description: Parses a string to count how many comments there are
* Author: Rick Caudill
* Date: Thu Mar 18 21:39:23 2004
************************************/
int GetCommentCount(const String& cLines)
{
	int nCommentCount=0;
	//TODO: Probably could use find instead of parsing every char
	for (uint i=0; i<cLines.size(); i++)
	{
		if (cLines[i] == '/')  //we found a slash
		{
			if (cLines[i+1] == '/') //we found another slash increment and then go on
			{
				nCommentCount++;
				i++;
			}
			
			else if (cLines[i+1] == '*') //we found a star now lets go on until there is a final /
			{
				nCommentCount++;
				i++;
				
				while (cLines[i] != '/')
				{
					if (cLines[i] == '\n')
						nCommentCount++;
					i++;
				}
			}
		}
	}
	return nCommentCount;
}

/********************************************
* Description: Converts an int into an os::String
* Author: Rick Caudill
* Date: Thu Mar 18 21:40:11 2004
*********************************************/
String ConvertIntToString(int nInput)
{
	char zReturn[255] = "";
	sprintf(zReturn,"%d",nInput);
	return (String(zReturn));
}

/*********************************************
* Description: Removes all carriage returns from file
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String ConvertCrLfToLf(const String& cCRLFString)
{
	uint nCurrentChar;
	String cLFString;

	cLFString="";

	for(nCurrentChar=0;nCurrentChar<cCRLFString.size();nCurrentChar++)
	{
		if(cCRLFString[nCurrentChar]!='\r')
			cLFString+=cCRLFString[nCurrentChar];
	}

	return(cLFString);
}

/*********************************************
* Description: Removes the files extension
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String RemoveExtension(const String& cFile)
{
	int nPos = cFile.find( "." );
	return nPos >= 0 ? cFile.substr( 0, nPos ) : cFile;
}

/*********************************************
* Description: Returns the directory of the opened file
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetDirectoryofFileOpened(const String& cFile)
{
	std::string cText = cFile.c_str();
	int nPos = cText.find_last_of("/");
	return cFile.substr(0,nPos);
}

/*********************************************
* Description: Returns just the filename(no directories)
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetFileNameOnly(const String& cFile)
{
	std::string cText = cFile.c_str();
	int nPos = cText.find_last_of("/");
	return cFile.substr(nPos+1,cFile.size());
}

/*********************************************
* Description: Parses 'Macros' and places them into the string
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String ParseForMacros(const String& cOld,SourceryFile* pcFile)
{
	String cReturn="";
	uint nCurrentChar;
	
	for (nCurrentChar=0; nCurrentChar<cOld.size(); nCurrentChar++)
	{
		if (cOld[nCurrentChar] == '%')
		{
			if (cOld[nCurrentChar+1] == 'D')
			{
				cReturn += GetDate();
				cReturn += " ";
				nCurrentChar +=2;
			}
			
			else if (cOld[nCurrentChar+1] == 'T')
			{
				cReturn += GetTime();
				cReturn += " ";
				nCurrentChar +=2;
			}
			
			else if (cOld[nCurrentChar+1] == 'N')
			{
				cReturn += GetName();
				cReturn += " ";
				nCurrentChar +=2;
			}
			
			else if (cOld[nCurrentChar+1] == 'S') //system info
			{
				cReturn += GetSystemInfo();
				cReturn += " ";
				nCurrentChar +=2;
			}
			
			else if (cOld[nCurrentChar+1] == 'F') //file name
			{
				if (pcFile != NULL)
				{
					cReturn += pcFile->GetFileName();
					cReturn += " ";
					nCurrentChar +=2;
				}
				
				else
				{
					cReturn += "invalid file";
					cReturn += " ";
					nCurrentChar +=2;
				}
			}
			
			else if (cOld[nCurrentChar+1] == 'C') //comment
			{
				if (pcFile != NULL)
				{
					cReturn += GetComment(pcFile->GetFilesExtension());
					cReturn += " ";
					nCurrentChar +=2;
				}
				
				else
				{
					cReturn += "#";
					cReturn += " ";
					nCurrentChar+=2;
				}
			}
			
			else
			{
				cReturn += String("%");
			}
		}
		else
		{
			cReturn += cOld[nCurrentChar];
		}
	}
	return cReturn;
}

/*********************************************
* Description: Parses for underscores
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String ParseForUnderScores(const String& cOld)
{
	String cReturn="";
	uint nCurrentChar;
	
	for (nCurrentChar=0; nCurrentChar<cOld.size(); nCurrentChar++)
	{
		if (cOld[nCurrentChar] == '_')
		{
			cReturn += "__";
		}
		else
			cReturn += cOld[nCurrentChar];
	}
	return cReturn;	
}

/*********************************************
* Description: Gets the system information
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetSystemInfo()
{
	String cReturn="";
	system_info sSysInfo;
	get_system_info(&sSysInfo);
	cReturn.Format("Syllable v%d.%d.%d built on %s %s for %s",( int )( ( sSysInfo.nKernelVersion >> 32 ) & 0xffff ), ( int )( ( sSysInfo.nKernelVersion >> 16 ) % 0xffff ), ( int )( sSysInfo.nKernelVersion & 0xffff ),sSysInfo.zKernelBuildDate,sSysInfo.zKernelBuildTime, sSysInfo.zKernelCpuArch);
	return cReturn;
}

/*********************************************
* Description: Gets the current users name
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetName()
{
	String cReturn = "";
	struct passwd *sPasswd;
	sPasswd = getpwuid(getgid());
	cReturn = sPasswd->pw_gecos;
	return cReturn;
}

/*********************************************
* Description: Reads the lines from the file
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetLinesFromFile(File* pcFile)
{
	size_t nSize = pcFile->GetSize();
	char zLines[nSize];
	pcFile->Read(zLines,nSize);
	zLines[nSize] = 0;
	return String(zLines);
}

/*********************************************
* Description: Returns the path of the Application
* Author: Rick Caudill
* Date: Sun Oct 10 00:39:58 2004
**********************************************/
String GetApplicationPath()
{
	String cPath;
	const unsigned BUFFER_SIZE = PATH_MAX+200;
	char buffer[BUFFER_SIZE];
	int nFD;
	
	if( (nFD = open_image_file( IMGFILE_BIN_DIR )) < 0 )
		return "";
		
	if( get_directory_path( nFD,buffer,BUFFER_SIZE ) < 0 )
		return "";
	
	close(nFD);
	buffer[BUFFER_SIZE-1] = 0; /* ensure null-terminated */
//	strcpy(buffer,buffer+5);  /* remove /boot */
	cPath =  String(buffer);
	
	if (cPath.find("/boot/syllable",0) == 0)
		cPath = cPath + 14;
	else if (cPath.find("/boot",0) == 0)
		cPath = cPath + 5;
		
	return cPath;
}	

