// sIDE		(C) 2001-2005 Arno Klenke
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

#include <fstream>
#include <gui/window.h>
#include <util/string.h>
#include <util/message.h>
#include <util/event.h>
#include <util/application.h>
#include <storage/path.h>
#include <storage/file.h>
#include <storage/registrar.h>
#include "project.h"
#include "messages.h"


#include <iostream>
#include <unistd.h>

project::project()
{
}

project::~project()
{
}

/* create a new project file */
void project::New( os::Path cFilePath )
{
	m_cFilePath = os::Path( cFilePath );
	m_zTarget = os::String( "App" );
	m_zCompilerFlags = os::String( "-Wall -c -O2 -fexceptions" );
	m_zLinkerFlags = "";
	m_zInstallPath = "/applications/App";
	m_zCategory = "Other";
	
	m_bIsWorking = false;
	m_bRunInTerminal = false;
	
	m_cGroups.clear();
}

/* read a string from a file until the end of the line */
os::String ReadString( os::File* pcFile )
{
	const char cLineEnd = 10;
	char cIn[2];
	cIn[1] = 0;
	os::String zReturn = os::String( "" );
		
	while( 1 )
	{
		if( pcFile->Read( &cIn[0], 1 ) < 1 ) 
			return( zReturn );
		if( cIn[0] == cLineEnd )
			return( zReturn );
		zReturn += cIn ;
	}
	
	return( zReturn );
}

/* read one tag from a file */
status_t ReadTag( os::File* pcFile, os::String* pzTag, os::String* pzValue )
{
	const char cLineEnd = 10;
	char cIn[2];
	cIn[1] = 0;
	*pzTag = os::String( "" );
	*pzValue = os::String( "" );
	bool bInTag = true;
		
	while( 1 )
	{
		if( pcFile->Read( &cIn[0], 1 ) < 1 )
			return( -1 );
		if( cIn[0] == cLineEnd )
			return( 0 );
		if( cIn[0] == '<' || cIn[0] == '>' )
		{
			if( !bInTag )
				return( -1 );
			else {
				if( cIn[0] == '>' )
					bInTag = false;
				continue;
			}
		}
		
		if( bInTag )
			*pzTag += cIn;
		else
			*pzValue += cIn;
	}
	return( -1 );
}

/* write one tag to a file */
status_t WriteTag( os::File* pcFile, os::String zTag, os::String zValue )
{
	const char cLineEnd = 10;
	os::String zWriteTag = os::String( "<" ) + zTag + os::String( ">" ) + zValue;
	if( pcFile->Write( zWriteTag.c_str(), zWriteTag.Length() ) != (int)zWriteTag.Length() )
		return( -1 );
	pcFile->Write( &cLineEnd, 1 );
	return( 0 );
}

/* load a project file with file version 1 */
bool project::LoadVersion1( os::Path cFilePath )
{
	New( cFilePath );
	const char cGroup = '*';
	const char cFileEnd = '|';
	os::String zIn;
	char cVersion;
	int nGroup = -1;
	int nType;
	
	/* check file */
	std::ifstream in( cFilePath.GetPath().c_str() );
	if( !in )
		return( false );
	in.close();
	os::File cFile = os::File( cFilePath.GetPath() );
	
	/* load file version */
	zIn = ReadString( &cFile );
	cVersion = *zIn.c_str();
	
	if( cVersion > FILE_VERSION )
		return( false );
	std::cout<<"File version :"<<(int)cVersion<<std::endl;
	
	/* load flags */
	m_zCompilerFlags = ReadString( &cFile );
	
	/* load flags */
	m_zLinkerFlags = ReadString( &cFile );
	
	/* load target */
	m_zTarget = ReadString( &cFile );
	
	/* load group/file names and types */
	while( 1 )
	{
		zIn = ReadString( &cFile );
	
		if( *zIn.c_str() == cFileEnd )
			return( true );
		else if( zIn == os::String( "" ) )
			return( false );
		else if( *zIn.c_str() == cGroup ) {
			zIn = ReadString( &cFile );
			AddGroup( zIn );
			nGroup++;
		} else {
			nType = *ReadString( &cFile ).c_str();
			AddFile( nGroup, zIn, nType );
		}
	}
	m_cFilePath = cFilePath;
	return( true );
}



/* load a project file */
bool project::Load( os::Path cFilePath )
{
	New( cFilePath );
	os::String zIn;
	char cVersion;
	int nGroup = -1;
	os::String zTag = "";
	os::String zValue = "";
	
	/* check file */
	int nStringLength = 0;
	os::String zNormalized( PATH_MAX, 0 );
	int nFd = open( cFilePath.GetPath().c_str(), O_RDONLY );
	if( ( nStringLength = get_directory_path( nFd, (char*)zNormalized.c_str(), PATH_MAX ) ) < 0 )
	{
		if( nFd >= 0 )
			close( nFd );
		return( false );
	}
	close( nFd );
	zNormalized.Resize( nStringLength );
	cFilePath = zNormalized;
	
	os::File cFile;
	if( cFile.SetTo( cFilePath.GetPath() ) < 0 )
		return( false );
		
	if( cFile.IsDir() )
		return( false );
	
	/* load file version */
	zIn = ReadString( &cFile );
	cVersion = *zIn.c_str();
	
	if( cVersion == 1 )
		return( LoadVersion1( cFilePath ) );
	else if( cVersion > FILE_VERSION )
		return( false );
	std::cout<<"File version :"<<(int)cVersion<<std::endl;
	
	/* Load tags */
	bool bNameRead = false;
	os::String zFileName;
	os::String zFileType;
	while( ReadTag( &cFile, &zTag, &zValue ) == 0 )
	{
		//std::cout<<zTag.c_str()<<" "<<zValue.c_str()<<std::endl;
		if( zTag == "CompilerFlags" )
			m_zCompilerFlags = zValue;
		else if( zTag == "LinkerFlags" )
			m_zLinkerFlags = zValue;
		else if( zTag == "Target" )
			m_zTarget = zValue;
		else if( zTag == "Category" )
			m_zCategory = zValue;			
		else if( zTag == "InstallPath" )
			m_zInstallPath = zValue;
		else if( zTag == "RunInTerminal" )
			m_bRunInTerminal = ( zValue == "true" ) ? true : false;
		else if( zTag == "Group" ) {
			AddGroup( zValue );
			nGroup++;
		}
		else if( zTag == "FileName" ) {
			zFileName = zValue;
			bNameRead = true;
		}
		else if( zTag == "FileType" ) {
			if( bNameRead )
			{
				bNameRead = false;
				AddFile( nGroup, zFileName, zValue[0] );
			}
		}
	}
	m_cFilePath = cFilePath;
	return( true );
}


/* save the project file */
bool project::Save( os::Path cFilePath )
{
	uint i, j;
	const char cLineEnd = 10;
	const char cVersion = FILE_VERSION;
	
	if( cFilePath.GetLeaf() == "NewProject.side" )
		return( true );

	os::File cFile = os::File( cFilePath.GetPath(), O_RDWR | O_TRUNC | O_CREAT );
	
	/* Write attribute */
	os::String zMimeType = "application/x-side";
	cFile.WriteAttr( "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, zMimeType.c_str(), 0, zMimeType.Length() + 1 );

	/* save file version */
	cFile.Write( &cVersion, 1 );
	cFile.Write( &cLineEnd, 1 );
	
	/* save the flags */
	WriteTag( &cFile, "CompilerFlags", m_zCompilerFlags );
	WriteTag( &cFile, "LinkerFlags", m_zLinkerFlags );
	

	
	/* save the target and install path */
	WriteTag( &cFile, "Target", m_zTarget );
	WriteTag( &cFile, "Category", m_zCategory );
	WriteTag( &cFile, "InstallPath", m_zInstallPath );
	WriteTag( &cFile, "RunInTerminal", m_bRunInTerminal ? "true" : "false" );

	/* save the group/file names and types */
	for( i = 0; i < GetGroupCount(); i++ )
	{
		WriteTag( &cFile, "Group", GetGroupName( i ) );
		for( j = 0; j < GetFileCount( i ); j++ )
		{
			WriteTag( &cFile, "FileName", GetFileName( i, j ) );
			char cC[2];
			cC[0] = GetFileType( i, j );
			cC[1] = 0;
			WriteTag( &cFile, "FileType", os::String( cC ) );
		}
	}
	
	/* Write attribute */
	cFile.WriteAttr( "os::MimeType", O_TRUNC, ATTR_TYPE_STRING, "application/x-side", 0, sizeof( "application/x-side" ) );
	
	std::cout<<"Project "<<cFilePath.GetPath().c_str()<<" saved"<<std::endl;
	m_cFilePath = cFilePath;
	cFile.Unset();

	return( true );
}

/* close all project files */
void project::Close()
{
	m_bIsWorking = true;
	Save( m_cFilePath );
	m_bIsWorking = false;
}


/* Remove objects */
void project::Clean()
{
	os::String zLinkerFiles;
	
	/* Build path */
	os::String cPath = os::String( m_cFilePath.GetDir().GetPath() ) + os::String( "/" ) + os::String( "build.sh" );
	
	/* Open file */
	std::ofstream out( cPath.c_str() );
	if( !out ) 
		return;
	
	/* Create makefile */
	ExportMakefile();
	
	/* Write script */
	out<<"#!/bin/sh"<<std::endl;
	out<<"export PATH=$PATH:/resources/index/programs:/usr/bin"<<std::endl;
	out<<"cd '"<<m_cFilePath.GetDir().GetPath().c_str()<<"'"<<std::endl;
	out<<"make -s clean"<<std::endl;
	out<<"echo Finished - Press return to close this window"<<std::endl;
	out<<"read"<<std::endl;
	out.close();
	
	
	/* Run shell script in an aterm window */
	if( fork() == 0 )
	{
		set_thread_priority( -1, 0 );
		execlp( "aterm", "aterm", cPath.c_str(), (void*)NULL );
		exit( 0 );
	}
	
}

/* compile and link the executable */
void project::Compile()
{
	os::String zLinkerFiles;
	
	/* Build path */
	os::String cPath = os::String( m_cFilePath.GetDir().GetPath() ) + os::String( "/" ) + os::String( "build.sh" );
	
	/* Open file */
	std::ofstream out( cPath.c_str() );
	if( !out ) 
		return;
	
	/* Create makefile */
	ExportMakefile();
	
	/* Write script */
	out<<"#!/bin/sh"<<std::endl;
	out<<"export PATH=$PATH:/resources/index/programs:/usr/bin"<<std::endl;
	out<<"cd '"<<m_cFilePath.GetDir().GetPath().c_str()<<"'"<<std::endl;
	out<<"make"<<std::endl;
	out<<"echo Finished - Press return to close this window"<<std::endl;
	out<<"read"<<std::endl;
	out.close();
	
	
	/* Run shell script in an aterm window */
	if( fork() == 0 )
	{
		set_thread_priority( -1, 0 );
		execlp( "aterm", "aterm", cPath.c_str(), (void*)NULL );
		exit( 0 );
	}
	
}

/* start the executable of the project */
void project::Run()
{
	/* Build app path */
	os::String cAppPath = os::String( m_cFilePath.GetDir().GetPath() ) + os::String( "/" ) + m_zTarget;
		
	if( !m_bRunInTerminal )
	{
		/* Run the programm */
		if( fork() == 0 )
		{
			set_thread_priority( -1, 0 );
			execlp( cAppPath.c_str(), cAppPath.c_str(), (void*)NULL );
			exit( 0 );
		}
		return;
	}
	
	/* Run the program in the terminal */

	os::String cScriptPath = os::String( m_cFilePath.GetDir().GetPath() ) + os::String( "/" ) + os::String( "build.sh" );
	
	
	/* Open file */
	std::ofstream out( cScriptPath.c_str() );
	if( !out ) 
		return;
	
	/* Write script */
	out<<"#!/bin/sh"<<std::endl;
	out<<"export PATH=$PATH:/resources/index/programs:/usr/bin"<<std::endl;
	out<<"cd '"<<m_cFilePath.GetDir().GetPath().c_str()<<"'"<<std::endl;
	out<<"\""<<cAppPath.c_str()<<"\""<<std::endl;
	out.close();
	
	
	/* Run shell script in an aterm window */
	if( fork() == 0 )
	{
		set_thread_priority( -1, 0 );
		execlp( "aterm", "aterm", cScriptPath.c_str(), (void*)NULL );
		exit( 0 );
	}
	
}

/* export a Makefile for the project */
void project::ExportMakefile()
{
	uint i, j;
	bool bLib = false;
	
	/* Build path */
	os::String cPath = os::String( m_cFilePath.GetDir().GetPath() ) + os::String( "/" ) + os::String( "makefile" );
	
	/* Open file */
	std::ofstream out( cPath.c_str() );
	if( !out ) 
		return;
		
	/* Check if we want to build a library */
	if( m_zTarget.Length() > 3 && m_zTarget[m_zTarget.Length()-3] == '.' 
	    && m_zTarget[m_zTarget.Length()-2] == 's' && m_zTarget[m_zTarget.Length()-1] == 'o')
		bLib = true;
	
	std::cout<<"Creating Makefile "<<cPath.c_str()<<std::endl;
	
	
	out<<"#Makefile for "<<m_zTarget.c_str()<<std::endl;
	
	/* write flags */
	out<<"COPTS = "<<m_zCompilerFlags.c_str()<<std::endl;
	out<<std::endl;
	out<<"APPBIN = "<<m_zInstallPath.c_str()<<std::endl;
	
	/* write objects */
	out<<"OBJS =";
	for( i = 0; i < GetGroupCount(); i++ )
	{
		for( j = 0; j < GetFileCount( i ); j++ )
		{
			int nType = GetFileType( i, j );
			os::String zFile = GetFileName( i, j );
			if( nType == TYPE_C || nType == TYPE_CPP )
			{
				bool bConverted = false;
				for( uint i = zFile.Length() - 1; i > 0; i-- )
				{
					if( zFile[i] == '.' )
					{
						zFile = zFile.substr( 0, i ) + ".o";
						bConverted = true;
						break;
					}
				}
				if( bConverted )
					out<<" "<<zFile.c_str();
			}
		}
	}
	out<<std::endl;
	out<<std::endl;
	
	/* Add rules */
	out<<"OBJDIR := objs"<<std::endl;
	out<<"OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))"<<std::endl;
	out<<std::endl;
	out<<"# Rules"<<std::endl;
	out<<"$(OBJDIR)/%.o : %.c"<<std::endl;
	out<<"	@echo Compiling : $<"<<std::endl;
	out<<"	@$(CC) $(COPTS) $< -o $@"<<std::endl;
	out<<std::endl;
	out<<"$(OBJDIR)/%.o : %.cpp"<<std::endl;
	out<<"	@echo Compiling : $<"<<std::endl;
	out<<"	@$(CXX) $(COPTS) $< -o $@"<<std::endl;
	out<<std::endl;
	out<<"$(OBJDIR)/%.o : %.s"<<std::endl;
	out<<"	@echo Assembling : $<"<<std::endl;
	out<<"	@$(CC) $(COPTS) -x assembler-with-cpp $< -o $@"<<std::endl;
	out<<std::endl;
	
	/* Add libraries to the linker flags */
	os::String zLinkerFlags = m_zLinkerFlags;
	for( i = 0; i < GetGroupCount(); i++ )
	{
		for( j = 0; j < GetFileCount( i ); j++ )
		{
			int nType = GetFileType( i, j );
			if( nType == TYPE_LIB )
			{
				zLinkerFlags += os::String( " -l" ) + GetFileName( i, j );
			}
		}
	}
	
	
	/* Add target */
	out<<"all : translations objs \""<<m_zTarget.c_str()<<"\""<<std::endl;
	out<<std::endl;
	
	/* Add interface dependencies */
	for( i = 0; i < GetGroupCount(); i++ )
	{
		for( j = 0; j < GetFileCount( i ); j++ )
		{
			int nType = GetFileType( i, j );
			if( nType == TYPE_INTERFACE )
			{
				os::String zIFName = GetFileName( i, j );
				if( zIFName[zIFName.Length()-3] == '.' &&
					zIFName[zIFName.Length()-2] == 'i' &&
					zIFName[zIFName.Length()-1] == 'f' )
				{
					out<<"$(OBJDIR)/"<<zIFName.substr( 0, zIFName.Length() - 3 ).c_str()<<".o: "<<zIFName.substr( 0, zIFName.Length() - 3 ).c_str()<<"Layout.cpp"<<std::endl;
				}
			}
		}
	}
	out<<std::endl;
	
	/* Add catalog compiler commands */
	bool bCatalogsFound = false;
	out<<"translations:"<<std::endl;
	for( i = 0; i < GetGroupCount(); i++ )
	{
		for( j = 0; j < GetFileCount( i ); j++ )
		{
			int nType = GetFileType( i, j );
			if( nType == TYPE_CATALOG )
			{
				if( !bCatalogsFound ) {
					
					out<<"	@echo Creating catalogs..."<<std::endl;
				}
				if( os::Path( GetFileName( i, j ) ).GetDir().GetLeaf().Length() != 2 )
					out<<"	@catcomp -c \""<<GetFileName( i, j ).c_str()<<"\""<<std::endl;
				else				
					out<<"	@catcomp -t \""<<GetFileName( i, j ).c_str()<<"\""<<std::endl;
				bCatalogsFound = true;
			}
		}
	}
	out<<std::endl;
		
	out<<"objs:"<<std::endl;
	out<<"	@mkdir -p objs"<<std::endl;
	out<<std::endl;
	out<<"\""<<m_zTarget.c_str()<<"\": $(OBJS)"<<std::endl;
	out<<"	@echo Linking..."<<std::endl;
	if( bLib )
		out<<"	@$(CXX) -shared -Xlinker -soname=\""<<m_zTarget.c_str()<<"\" $(OBJS) -o \""<<m_zTarget.c_str()<<"\" "<<zLinkerFlags.c_str()<<std::endl;
	else
		out<<"	@$(CXX) $(OBJS) -o \""<<m_zTarget.c_str()<<"\" "<<zLinkerFlags.c_str()<<std::endl;
	
	/* Add resource entries */
	os::String zResources = os::String( "	@rescopy \"" ) + m_zTarget + os::String( "\" -r " );
	bool bResourceFound = false;
	for( i = 0; i < GetGroupCount(); i++ )
	{
		for( j = 0; j < GetFileCount( i ); j++ )
		{
			int nType = GetFileType( i, j );
			if( nType == TYPE_RESOURCE )
			{
				if( !bResourceFound ) {
					bResourceFound = true;
					out<<"	@echo Adding resources..."<<std::endl;
				}
				zResources += GetFileName( i, j ) + os::String( " " );
			}
		}
	}
	if( bResourceFound )
		out<<zResources.c_str()<<std::endl;
		
	/* Add catalog entries */
	bCatalogsFound = false;
	for( i = 0; i < GetGroupCount(); i++ )
	{
		for( j = 0; j < GetFileCount( i ); j++ )
		{
			int nType = GetFileType( i, j );
			if( nType == TYPE_CATALOG )
			{
				if( !bCatalogsFound ) {
					out<<"	@echo Adding catalogs..."<<std::endl;
				}
				os::String cCatalogFile = GetFileName( i, j );
				/* Change extension */
				cCatalogFile = cCatalogFile.substr( 0, cCatalogFile.Length() - 2 );
				cCatalogFile += "catalog";
				if( os::Path( cCatalogFile ).GetDir().GetLeaf().Length() != 2 )
					out<<"	@rescopy \""<<m_zTarget.c_str()<<( ( !bCatalogsFound && !bResourceFound ) ? "\" -r " : "\" -a ")<<cCatalogFile.c_str()<<std::endl;
				else				
					out<<"	@rescopy \""<<m_zTarget.c_str()<<( ( !bCatalogsFound && !bResourceFound ) ? "\" -r " : "\" -a ")<<
				os::Path( cCatalogFile ).GetDir().GetLeaf().c_str()<<"/"<<os::Path( cCatalogFile ).GetLeaf().c_str()<<"="<<cCatalogFile.c_str()<<std::endl;
				bCatalogsFound = true;
			}
		}
	}
	
		
	/* Add category */
	out<<"	@addattrib \""<<m_zTarget.c_str()<<"\" os::Category "<<m_zCategory.c_str()<<std::endl;
	
	out<<std::endl;
	out<<"clean:"<<std::endl;
	out<<"	@echo Cleaning..."<<std::endl;
	out<<"	@rm -f $(OBJDIR)/*"<<std::endl;
	out<<"	@rm -f \""<<m_zTarget.c_str()<<"\""<<std::endl;
	out<<std::endl;
	out<<"deps:"<<std::endl;
	out<<std::endl;
	out<<"install: all"<<std::endl;
	out<<"	@echo Installing..."<<std::endl;
	out<<"	@mkdir -p $(IMAGE)"<<m_zInstallPath.c_str()<<std::endl;
	out<<"	@cp \""<<m_zTarget.c_str()<<"\" \"$(IMAGE)"<<m_zInstallPath.c_str()<<"/"<<m_zTarget.c_str()<<"\""<<std::endl;
	out<<std::endl;
	out<<"dist: install"<<std::endl;
	out<<std::endl;
	
	out.close();
}

/* get the path of the project file */
os::Path project::GetFilePath()
{
	return( m_cFilePath );
}

/* get the target executable of the project */
os::String project::GetTarget()
{
	return( m_zTarget );
}

/* set the target executable of the project */
void project::SetTarget( os::String zName )
{
	m_zTarget = zName;
}

/* get the target category */
os::String project::GetCategory()
{
	return( m_zCategory );
}

/* set the category of the executable */
void project::SetCategory( os::String zCategory )
{
	m_zCategory = zCategory;
}

/* get the install path of the project */
os::String project::GetInstallPath()
{
	return( m_zInstallPath );
}

/* set the target executable of the project */
void project::SetInstallPath( os::String zName )
{
	m_zInstallPath = zName;
}

/* get the count of groups */
uint project::GetGroupCount()
{
	return( m_cGroups.size() );
}

/* get the name of a group */
os::String project::GetGroupName( uint nNumber )
{
	if( nNumber >= m_cGroups.size() )
		return( "" );
	return( m_cGroups[nNumber].m_zName );
}

/* get the count of files in one group */
uint project::GetFileCount( uint nGroup )
{
	if( nGroup >= m_cGroups.size() )
		return( 0 );
	return( m_cGroups[nGroup].m_cFiles.size() );
}

/* get the project_file structure of a file */
ProjectFile* project::GetFile( uint nGroup, uint nNumber )
{
	if( nGroup >= m_cGroups.size() )
		return( NULL );
	if( nNumber >= m_cGroups[nGroup].m_cFiles.size() )
		return( NULL );

	return( &m_cGroups[nGroup].m_cFiles[nNumber] );
}

/* get name of a file */
os::String project::GetFileName( uint nGroup, uint nNumber )
{
	if( nGroup >= m_cGroups.size() )
		return( "" );
	if( nNumber >= m_cGroups[nGroup].m_cFiles.size() )
		return( "" );

	return( m_cGroups[nGroup].m_cFiles[nNumber].m_zName );
}

/* get type of a file */
char project::GetFileType( uint nGroup, uint nNumber )
{
	if( nGroup >= m_cGroups.size() )
		return( TYPE_NONE );
	if( nNumber >= m_cGroups[nGroup].m_cFiles.size() )
		return( TYPE_NONE );

	return( m_cGroups[nGroup].m_cFiles[nNumber].m_nType );
}

/* add a group */
void project::AddGroup( os::String zName )
{
	ProjectGroup cGroup;
	cGroup.m_zName = zName;
	cGroup.m_cFiles.clear();
	m_cGroups.push_back( cGroup );
}

/* add a file to a group */
void project::AddFile( uint nGroup, os::String zName, uint nType )
{
	if( nGroup >= m_cGroups.size() )
		return;
		
	ProjectFile cFile;
	cFile.m_nType = nType;
	cFile.m_zName = os::String( zName );
	cFile.m_bIsCompiled = false;
	m_cGroups[nGroup].m_cFiles.push_back( cFile );
}

/* Move a group */
void project::MoveGroup( uint nOld, uint nNew )
{
	if( nOld >= m_cGroups.size() || nNew >= m_cGroups.size() )
		return;
	ProjectGroup cTemp = m_cGroups[nNew];
	m_cGroups[nNew] = m_cGroups[nOld];
	m_cGroups[nOld] = cTemp;
}

/* remove a group and all the files of this group */
void project::RemoveGroup( uint nNumber )
{
	if( nNumber >= m_cGroups.size() )
		return;
	m_cGroups.erase( m_cGroups.begin() + nNumber );
}

/* remove a file from a group */
void project::RemoveFile( uint nGroup, uint nNumber )
{
	if( nGroup >= m_cGroups.size() )
		return;
	if( nNumber >= m_cGroups[nGroup].m_cFiles.size() )
		return;
	m_cGroups[nGroup].m_cFiles.erase( m_cGroups[nGroup].m_cFiles.begin() + nNumber );
	
}

/* open a window for a file */
void project::OpenFile( uint nGroup, uint nNumber, os::Window *pcWindow )
{
	ProjectFile* pcFile;
	
	if( nGroup >= m_cGroups.size() )
		return;
	if( nNumber >= m_cGroups[nGroup].m_cFiles.size() )
		return;
		
	pcFile = &m_cGroups[nGroup].m_cFiles[nNumber];
	/* Build path */
	os::String cPath = os::String( m_cFilePath.GetDir().GetPath() ) + os::String( "/" ) + pcFile->m_zName;	
	if( pcFile->m_nType < TYPE_LIB ) {						
		/* Create the file if it does not exist yet */
		os::File cFile( cPath, O_RDWR | O_CREAT );
		cFile.Flush();
		cFile.Unset();
		
		/* Check if the file is already opened */
		os::Event cEvent;
		int i = 0;
		os::Message cReply;
		cEvent.SetToRemote( "app/Sourcery/GetFilePath", -1 );
		if( cEvent.GetLastEventMessage( &cReply ) == 0 )
		{
			os::Message cMsg;
			while( cReply.FindMessage( "last_message", &cMsg, i ) == 0 )
			{
				os::String zPath;
				if( cMsg.FindString( "file/path", &zPath ) == 0 )
				{
					if( zPath == cPath )
					{
						/* Just focus the window */
						if( cEvent.SetToRemote( "app/Sourcery/MakeFocus", i ) == 0 )
						{
							cEvent.PostEvent( &cMsg );
							return;
						}
					}
				}
				i++;
			}
		}
				
		if( fork() == 0 )
		{
			set_thread_priority( -1, 0 );
			execlp( "/applications/sIDE/Sourcery", "/applications/sIDE/Sourcery", cPath.c_str(), (void*)NULL );
			exit( 0 );
		}
	} else if( pcFile->m_nType == TYPE_CATALOG || pcFile->m_nType == TYPE_RESOURCE || pcFile->m_nType == TYPE_OTHER ) {
		/* Try to open it using the registrar */
		os::RegistrarManager* pcManager = os::RegistrarManager::Get();
		pcManager->Launch( NULL, cPath, true );
		pcManager->Put();
	} else if( pcFile->m_nType == TYPE_INTERFACE ) {
		os::File cFile( cPath, O_RDWR | O_CREAT );
		cFile.Flush();
		cFile.Unset();
		/* Try to open it using the registrar */
		os::RegistrarManager* pcManager = os::RegistrarManager::Get();
		pcManager->Launch( NULL, cPath, true );
		pcManager->Put();
	}
	
	
}

bool project::IsWorking()
{
	return( m_bIsWorking );
}
