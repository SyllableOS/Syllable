/*
    launcher_plugin - A plugin interface for the AtheOS Launcher
    Copyright (C) 2002 Andrew Kennan (bewilder@stresscow.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    See ../../include/COPYING
*/

#include "launcher_mime.h"



MimeInfo::MimeInfo( )
{
	//m_cMimeTypePath = get_launcher_path() + "mime/";
	//m_cDefaultIconPath = get_launcher_path() + "icons/file.png";
	
	m_cMimeType = "application/octet-stream";
	//m_cDefaultIcon = m_cMimeTypePath;
	m_cDefaultCommand = "%f";
}



MimeInfo::MimeInfo( const std::string &cMimeType )
{
	//m_cMimeTypePath = get_launcher_path() + "mime/";
	//m_cDefaultIconPath = get_launcher_path() + "icons/file.png";

	if( MimeTypeLooksValid( cMimeType ) ) {
		SetMimeType( cMimeType );
	} else 
		delete this;
}



MimeInfo::~MimeInfo( )
{ }



void MimeInfo::SetMimeType( const std::string &cMimeType )
{
	if( ! ReadMimeType( cMimeType ) ) {
		if( MimeTypeLooksValid( cMimeType ) ) {
			m_cMimeType = cMimeType;
			m_cDefaultIcon = m_cDefaultIconPath;
			m_cDefaultCommand = "%f";
			WriteMimeType( );
		}
	}
}



std::string MimeInfo::GetMimeType( void )
{
	return m_cMimeType;
}



void MimeInfo::SetDefaultCommand( const std::string &cCommand )
{
	m_cDefaultCommand = cCommand;
	WriteMimeType();
}



std::string MimeInfo::GetDefaultCommand( void )
{
	return m_cDefaultCommand;
}



void MimeInfo::SetDefaultIcon( const std::string &cIcon )
{
	m_cDefaultIcon = cIcon;
	WriteMimeType();
}



std::string MimeInfo::GetDefaultIcon( void )
{
	return m_cDefaultIcon;
}



void MimeInfo::AddNameMatch( const std::string &cRegex )
{
	bool bFound = false;
	for( uint n = 0; n < m_acMatch.size(); n++ )
		if( m_acMatch[n] == cRegex )
			bFound = true;
			
	if( ! bFound ) {
		m_acMatch.push_back( cRegex );
		WriteMimeType();
	}
}



void MimeInfo::DelNameMatch( const std::string &cRegex )
{
	bool bFound = false;
	for( uint n = 0; n < m_acMatch.size(); n++ )
		if( m_acMatch[n] == cRegex ) {
			m_acMatch[n] = "";
			bFound = true;
		}
		
	if( bFound )
		WriteMimeType();
}



std::vector<std::string> MimeInfo::GetNameMatches( void )
{
	return m_acMatch;
}



int MimeInfo::FindMimeType( const std::string &cFile )
{
	FSNode *pcNode = new FSNode();
	if( pcNode->SetTo( cFile ) == 0 ) {
		struct stat sStat;	
		pcNode->GetStat( &sStat );
		if( S_ISDIR( sStat.st_mode ) ) {
			ReadMimeType( "dir" );
			delete pcNode;
			return 0;
		}
	}
	delete pcNode;
	
	if( m_cMimeType != "" )
		if( FileMatches( cFile ) )
			return 0;
		
	std::string cOldMimeType = m_cMimeType;
	Directory *pcMimeDir = new Directory();
	Directory *pcMajor = new Directory();
	if( pcMimeDir->SetTo( m_cMimeTypePath ) != 0 )
		return -1;
		
	std::string cMajorName = "";
	std::string cMinorName = "";
	while( pcMimeDir->GetNextEntry( &cMajorName ) ) 
		if( cMajorName.substr(0,1) != "." )
			if( pcMajor->SetTo( m_cMimeTypePath + cMajorName ) == 0 ) 
				while( pcMajor->GetNextEntry( &cMinorName ) )
					if( cMinorName.substr(0,1) != "." ) {
						string cTempType = cMajorName + (std::string)"/" + cMinorName;
						if( cTempType != cOldMimeType )
							if ( ReadMimeType( cTempType ) )
								if( FileMatches( cFile ) ) {
									delete pcMimeDir;
									delete pcMajor;
									return 0;
								}
						
					}
					
	ReadMimeType( "application/octet-stream" );
	return 0;
}



bool MimeInfo::FileMatches( const std::string &cFile )
{
	FSNode *pcFile = new FSNode();
	if( pcFile->SetTo( cFile ) == 0 ) {
		char zBuf[512];
		uint nSize = pcFile->ReadAttr( "mime-type", ATTR_TYPE_STRING, zBuf, 0, 512 );
		if( nSize > 0 ) {
			zBuf[nSize] = 0;
			if( m_cMimeType == (string)zBuf ) {
				delete pcFile;
				return true;
			}
		}
	}
	
	RegExp *pcRegex = new RegExp( );
	for( uint n = 0; n < m_acMatch.size(); n++ ) {
		pcRegex->Compile( m_acMatch[n] );
		if( pcRegex->Search( cFile ) ) {
			pcFile->WriteAttr( "mime-type", O_TRUNC, ATTR_TYPE_STRING, m_cMimeType.c_str(), 0, m_cMimeType.length() );
			delete pcFile;
			delete pcRegex;
			return true;
		}
	}
	
	delete pcFile;	
	delete pcRegex;
	
	return false;
}



bool MimeInfo::MimeTypeLooksValid( const std::string &cMimeType )
{
	uint nSlash = cMimeType.find( "/" );
	if( nSlash == std::string::npos )
		return false;
		
	std::string cMajor = m_cMimeTypePath + cMimeType.substr( 0, nSlash );
	FSNode *pcMNode = new FSNode();
	if( pcMNode->SetTo( cMajor ) != 0 ) {
		delete pcMNode;
		return false;
	}
	
	delete pcMNode;
	return true;
}



bool MimeInfo::ReadMimeType( const std::string &cMimeType )
{
	
	std::string cFullPath = m_cMimeTypePath + cMimeType;
	FSNode *pcNode = new FSNode( );
	if( pcNode->SetTo( cFullPath ) != 0 )
		return false;
		
	m_cMimeType = cMimeType;
	m_cDefaultCommand = "";
	m_cDefaultIcon = "";
	m_acMatch.clear();
		
	int32 nAttrDate;
	int n = pcNode->ReadAttr( "InfoInAttr", ATTR_TYPE_INT32, &nAttrDate, 0, 4 );
	if( (n == -1) || (nAttrDate > pcNode->GetMTime()) ) {
		
		File *pcFile = new File( *pcNode );
		int nSize = pcFile->GetSize();
		char nBuf[nSize + 1];
		pcFile->Read( &nBuf, nSize );
		nBuf[nSize] = 0;
		std::string cContents = (std::string)nBuf;
		
		int nPos = 0;
		int nPrevPos = 0;
		while( nPos != -1 ) {
			nPos = cContents.find( "\n", nPrevPos );
			std::string cLine = cContents.substr( nPrevPos, nPos - nPrevPos );
			uint nEqPos = cLine.find( "=", 0 );
			if( nEqPos != std::string::npos ) {
				std::string cKey = cLine.substr(0,nEqPos);
				std::string cValue = cLine.substr( nEqPos + 1, cLine.length() - nEqPos );
				
				if( cKey == "DefaultCommand" )
				{ m_cDefaultCommand = cValue; }
				else if( cKey == "DefaultIcon" )
				{ m_cDefaultIcon = cValue; }
				else if( cKey == "Match" )
				{ m_acMatch.push_back( cValue ); }
					
			}
			nPrevPos = nPos + 1;
		}
		
		delete pcFile;
		delete pcNode;
		WriteMimeType( false );
	} else {
		char nBuf[1024];
		
		int nSize = pcNode->ReadAttr( "DefaultCommand", ATTR_TYPE_STRING, &nBuf, 0, 1023 );
		nBuf[nSize] = 0;
		m_cDefaultCommand = (std::string)nBuf;
		
		nSize = pcNode->ReadAttr( "DefaultIcon", ATTR_TYPE_STRING, &nBuf, 0, 1023 );
		nBuf[nSize] = 0;
		m_cDefaultIcon = (std::string)nBuf;
		
		std::string cAttrName;
		while( pcNode->GetNextAttrName( &cAttrName ) != 0 )
			if( cAttrName.substr( 0, 6 ) == "Match_" ) {
				nSize = pcNode->ReadAttr( cAttrName, ATTR_TYPE_STRING, nBuf, 0, 1023 );
				if( nSize != 0 ) {
					nBuf[nSize] = 0;
					m_acMatch.push_back( (std::string)nBuf );
				}
			}	
		delete pcNode;
	}	

	return true;
}



bool MimeInfo::WriteMimeType( bool bFileAndAttrs = true )
{
	std::string cFullPath = m_cMimeTypePath + m_cMimeType;
	FSNode *pcNode = new FSNode( );
	
	if( bFileAndAttrs ) {
		if( pcNode->SetTo( cFullPath, O_WRONLY | O_TRUNC ) != 0 )
			return false;

		File *pcFile = new File( *pcNode );
		
		std::string cLine = (std::string)"DefaultCommand=" + m_cDefaultCommand + (std::string)"\n";
		pcFile->Write( cLine.c_str(), cLine.length() );
		
		cLine = (std::string)"DefaultIcon=" + m_cDefaultIcon + (std::string)"\n";
		pcFile->Write( cLine.c_str(), cLine.length() );
		
		for( uint n = 0; n < m_acMatch.size(); n++ ) {
			cLine = (std::string)"Match=" + m_acMatch[n] + (std::string)"\n";
			pcFile->Write( cLine.c_str(), cLine.length() );
		}
		
		delete pcFile;
	} else if( pcNode->SetTo( cFullPath ) != 0 )
		return false;
		
	
	pcNode->WriteAttr( "DefaultCommand", O_TRUNC, ATTR_TYPE_STRING, m_cDefaultCommand.c_str(), 0, m_cDefaultCommand.length() );
	pcNode->WriteAttr( "DefaultIcon", O_TRUNC, ATTR_TYPE_STRING, m_cDefaultIcon.c_str(), 0, m_cDefaultIcon.length() );
	
	for( uint n = 0; n < m_acMatch.size(); n++ ) {
		char nBuf[10];
		sprintf( nBuf, "Match_%02d", n );
		std::string cAttrName = (std::string)nBuf;
		pcNode->WriteAttr( cAttrName, O_TRUNC, ATTR_TYPE_STRING, m_acMatch[n].c_str(), 0, m_acMatch[n].length() );
	}
	
	uint32 nSysTime = get_real_time( ) / 1000000;
	pcNode->WriteAttr( "InfoInAttr", O_TRUNC, ATTR_TYPE_INT32, &nSysTime, 0, 4 );
	
	delete pcNode;
	
	return true;
}


//**********************************************************************

MimeNode::MimeNode( ) : FSNode()
{
	m_pcMimeInfo = new MimeInfo();
}



MimeNode::MimeNode( const std::string &cPath, int nOpenMode = O_RDONLY ) :
	FSNode( cPath, nOpenMode )
{
	m_cPath = cPath;
	m_pcMimeInfo = new MimeInfo();
	if( m_pcMimeInfo->FindMimeType( cPath ) != 0)
		m_pcMimeInfo->SetMimeType( "application/octet-stream" );
		
}



MimeNode::~MimeNode()
{}



int MimeNode::SetTo( const std::string &cPath )
{
	int nResult = FSNode::SetTo( cPath );
	if( nResult == 0 ) {
		if( m_pcMimeInfo->FindMimeType( cPath ) != 0 )
			m_pcMimeInfo->SetMimeType( "application/octet-stream" );
		m_cPath = cPath;
	}
	return nResult;
}
 
 

const std::string MimeNode::GetMimeType( void )
{
	return m_pcMimeInfo->GetMimeType();
}



void MimeNode::SetMimeType( const std::string &cMimeType )
{
	FSNode *pcNode = new FSNode( );
	if( pcNode->SetTo( m_cPath ) == 0 ) {
		pcNode->WriteAttr( "mime-type", O_TRUNC, ATTR_TYPE_STRING, cMimeType.c_str(), 0 , cMimeType.length() );
		m_pcMimeInfo->FindMimeType( m_cPath );
	}

}



const std::string MimeNode::GetDefaultCommand( void )
{
	return m_pcMimeInfo->GetDefaultCommand();
}



const std::string MimeNode::GetDefaultIcon( void )
{
	return m_pcMimeInfo->GetDefaultIcon();
}



MimeInfo *MimeNode::GetMimeInfo( void )
{
	return m_pcMimeInfo;
}



void MimeNode::Launch( void )
{
	std::string cCommand = m_pcMimeInfo->GetDefaultCommand();
	
	if( cCommand != "" ) {
		std::vector<std::string> acArg;
		int nPos = 0;
		int nPrevPos = 0;
		while( nPos != -1 ) {
			nPos = cCommand.find( " ", nPrevPos );
			acArg.push_back( cCommand.substr( nPrevPos, nPos - nPrevPos ) );
			nPrevPos = nPos + 1;
		}
		
		char *apzArg[acArg.size() + 1];
		for( uint n = 0; n < acArg.size(); n++ ) {
			if( acArg[n] == "%f" ) {
				acArg[n] = m_cPath;
			}
			apzArg[n] = (char *)acArg[n].c_str();
		}
		apzArg[acArg.size()] = NULL;
		
		pid_t nPid = fork();
		if( nPid == 0 ) {
			set_thread_priority( -1, 0 );
			execvp( apzArg[0], apzArg );
			exit( 1 );
		}
	}
}	
	
	

