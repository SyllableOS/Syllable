/*  Syllable Dock
 *  Copyright (C) 2003 Arno Klenke
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */
 
#include <gui/menu.h>
#include <gui/image.h>
#include <storage/directory.h>
#include <storage/fsnode.h>
#include <storage/file.h>
#include <util/string.h>
#include <util/resources.h>
#include "DirMenu.h"
#include <storage/registrar.h>


DirMenu::DirMenu( os::Handler* pcHandler, os::Path cPath, os::Rect cFrame, const char* pzName, os::MenuLayout_e eLayout )
	 	: os::Menu( cFrame, pzName, eLayout )
{
	/* Check if the directory is valid */
	os::FSNode* pcNode = NULL;
	try
	{
		pcNode = new os::FSNode( cPath.GetPath() );
	} catch( ... ) {
		return;
	}
	if( !pcNode->IsValid() )
	{
		delete( pcNode );
		return;
	}
	delete( pcNode );
	
	/* Iterate through the directory */
	os::Directory cDir( cPath.GetPath() );
	cDir.Rewind();
	os::String zFile;
	
	/* Get the registrar manager */
	os::RegistrarManager* pcManager = NULL;
	try
	{
		pcManager = os::RegistrarManager::Get();
	} catch( ... )
	{
	}
	
	while( cDir.GetNextEntry( &zFile ) == 1 )
	{
		if( zFile == os::String( "Launcher1.cfg" ) ||
			zFile == os::String( "." ) ||
			zFile == os::String( ".." ) )
			continue;
		os::Path cFilePath = cPath;
		
		cFilePath.Append( zFile.c_str() );
		
		os::Image* pcItemIcon = NULL;
		
		/* Get icon */
		if( pcManager )
		{
			os::String zTemp;
			if( pcManager->GetTypeAndIcon( cFilePath, os::Point( 24, 24 ), &zTemp, &pcItemIcon ) < 0 )
				continue;
		}
		
		
		
		/* Validate entry */
		try {
			pcNode = new os::FSNode( cFilePath );
		} catch( ... ) {
			continue;
		}
		if( !pcNode->IsValid() )
		{
			delete( pcNode );
			continue;
		}
			
		if( pcNode->IsFile() )
		{
			/* Create an menu item */
			os::Message *pcMsg = new os::Message( 10002 );
			pcMsg->AddString( "file/path", cFilePath.GetPath() );
			
			AddItem( new os::MenuItem( zFile.c_str(), pcMsg, "", pcItemIcon ) );
		}
		if( pcNode->IsDir() )
		{
			/* Create another DirMenu */
			DirMenu* pcMenu = new DirMenu( pcHandler, cFilePath, os::Rect(), zFile.c_str(), os::ITEMS_IN_COLUMN ) ;
			AddItem( new os::MenuItem( pcMenu, NULL, "", pcItemIcon ) );
		}
		delete( pcNode );
	}
	
	if( pcManager )
		pcManager->Put();
	
	/* Create a nodemonitor for this directory */
	m_pcMonitor = new os::NodeMonitor( cPath.GetPath(), NWATCH_ALL, pcHandler );
}

DirMenu::~DirMenu()
{
	delete( m_pcMonitor );
}

























