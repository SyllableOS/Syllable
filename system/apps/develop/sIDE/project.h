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

#ifndef _PROJECT_H_
#define _PROJECT_H_

#define TYPE_NONE		0
#define TYPE_HEADER		1
#define TYPE_C			2
#define TYPE_CPP		3
#define TYPE_LIB		4
#define TYPE_OTHER		5
#define TYPE_RESOURCE	6
#define TYPE_CATALOG	7
#define TYPE_INTERFACE	8

#define FILE_VERSION	2

struct ProjectFile
{
	os::String			m_zName;
	char 				m_nType;
	bool				m_bIsCompiled;	
};

struct ProjectGroup
{
	os::String			m_zName;
	std::vector<ProjectFile> m_cFiles;
};

class project
{
	public:
		project();
		~project();
		
		void			New( os::Path cFilePath );
		
		bool			LoadVersion1( os::Path cFilePath );
		bool			Load( os::Path cFilePath );
		bool			Save( os::Path cFilePath );
		
		void			Close();
		
		void			Clean();
		void			Compile();
		void			Run();
		void			ExportMakefile();
		
		os::Path		GetFilePath();
		
		os::String		GetTarget();
		void			SetTarget( os::String zName );
		
		os::String		GetCategory();
		void			SetCategory( os::String zName );
		
		os::String		GetInstallPath();
		void			SetInstallPath( os::String zName );
		
		os::String		GetCFlags() { return( m_zCompilerFlags ); }
		void			SetCFlags( os::String zFlags ) { m_zCompilerFlags = zFlags; }
		
		os::String		GetLFlags() { return( m_zLinkerFlags ); }
		void			SetLFlags( os::String zFlags ) { m_zLinkerFlags = zFlags; }
		
		bool			GetTerminalFlag() { return( m_bRunInTerminal ); }
		void			SetTerminalFlag( bool bFlag ) { m_bRunInTerminal = bFlag; }
		
		uint			GetGroupCount();
		os::String		GetGroupName( uint nNumber );
		
		
		uint			GetFileCount( uint nGroup );
		ProjectFile*	GetFile( uint nGroup, uint nNumber );
		os::String		GetFileName( uint nGroup, uint nNumber );
		char			GetFileType( uint nGroup, uint nNumber );
		
		void			AddGroup( os::String zName );
		void			AddFile( uint nGroup, os::String zName, uint nType );
		void			MoveGroup( uint nOld, uint nNew );
		
		void			RemoveGroup( uint nNumber );
		void			RemoveFile( uint nGroup, uint nNumber ); 
		
		void			OpenFile( uint nGroup, uint nNumber, 
									os::Window *pcWindow );

		bool			IsWorking();

	private:
						
		os::Path		m_cFilePath;
		std::vector<ProjectGroup> m_cGroups;
		os::String		m_zCompilerFlags;	
		os::String		m_zLinkerFlags;
		os::String		m_zTarget;	
		os::String		m_zCategory;
		os::String		m_zInstallPath;
		bool			m_bRunInTerminal;
		bool			m_bIsWorking;	
};

#endif













