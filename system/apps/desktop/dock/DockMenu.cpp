/*  Syllable Dock
 *  Copyright (C) 2005 Arno Klenke
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
#include "DockMenu.h"
#include <storage/registrar.h>
#include <cassert>

DockMenu::DockMenu( os::Handler* pcHandler, os::Rect cFrame, const char* pzName, os::MenuLayout_e eLayout )
	 	: os::Menu( cFrame, pzName, eLayout )
{
	ScanPath( 1, os::Path( "/boot/atheos/Applications" ) );
	/* Create a nodemonitor for this directory */
	m_pcMonitor = new os::NodeMonitor( os::Path( "/boot/atheos/Applications" ), NWATCH_ALL, pcHandler );
}

DockMenu::~DockMenu()
{
	delete( m_pcMonitor );
}

/* The FindItem() method of the Menu class does not return submenus */
os::MenuItem* DockMenu::FindMenuItem( os::String zName )
{
	for( int i = 0; i < GetItemCount(); i++ )
	{
		os::MenuItem* pcItem = GetItemAt( i );
		if( pcItem && pcItem->GetLabel() == zName && ( pcItem->GetSubMenu() != NULL ) )
			return( pcItem );
	}
	return( NULL );
}

void DockMenu::AddEntry( os::RegistrarManager* pcManager, os::String zCategory, os::Path cPath, bool bUseDirIcon, os::Path cDir )
{
	os::String zTemp;
	os::Image* pcItemIcon = NULL;
	os::Image* pcCategoryIcon = NULL;
	
	
	/* Get the icons */
	if( pcManager )
	{
		os::String zTemp;
		if( pcManager->GetTypeAndIcon( cPath, os::Point( 24, 24 ), &zTemp, &pcItemIcon ) < 0 )
			pcItemIcon = NULL;
		if( bUseDirIcon )
			if( pcManager->GetTypeAndIcon( cDir, os::Point( 24, 24 ), &zTemp, &pcCategoryIcon ) < 0 )
				pcCategoryIcon = NULL;
	}
	
	os::Menu* pcAddMenu = this;
	if( zCategory != "" )
	{
		/* Create category if necessary */
		os::MenuItem* pcSubMenuItem = FindMenuItem( zCategory );
		if( !pcSubMenuItem )
		{
			if( pcCategoryIcon == NULL )
			{
				os::StreamableIO *pcStream;
				try
				{
					pcStream = new os::File( "/system/icons/folder.png" );
				} catch( ... )
				{
					os::File cSelf( open_image_file( get_image_id() ) );
					os::Resources cCol( &cSelf );		
					pcStream = cCol.GetResourceStream( "folder.png" );
				}

				os::BitmapImage* pcImg = new os::BitmapImage();
				pcImg->Load( pcStream );
				pcImg->SetSize( os::Point( 24, 24 ) );
				pcCategoryIcon = pcImg;
				delete( pcStream );
			}
			os::Menu* pcMenu = new os::Menu( os::Rect(), zCategory, os::ITEMS_IN_COLUMN );
			AddItem( new os::MenuItem( pcMenu, NULL, "", pcCategoryIcon ) );
			assert( ( pcSubMenuItem = FindMenuItem( zCategory ) ) != NULL );
		}
		if( pcSubMenuItem->GetSubMenu() )
			pcAddMenu = pcSubMenuItem->GetSubMenu();
	}
	/* Create the menu item */
	os::Message *pcMsg = new os::Message( 10002 );
	pcMsg->AddString( "file/path", cPath.GetPath() );		
	pcAddMenu->AddItem( new os::MenuItem( cPath.GetLeaf(), pcMsg, "", pcItemIcon ) );
}

void DockMenu::ScanPath( int nLevel, os::Path cPath )
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
		if( zFile == os::String( "." ) ||
			zFile == os::String( ".." ) )
			continue;
		os::Path cFilePath = cPath;
		
		/* We do not want builder or any plugins */
		if( zFile == "Builder" || zFile == "Plugins" )
			continue;
		
		cFilePath.Append( zFile.c_str() );
		
		os::Image* pcItemIcon = NULL;
		
		/* Get icon */
		os::FSNode cFileNode;
		if( cFileNode.SetTo( cFilePath ) != 0 )
		{
			continue;
		}
		
		if( cFileNode.IsDir() )
		{
			cFileNode.Unset();
			/* Recursive scan (depth of scan is limited) */
			if( nLevel < 5 )
				ScanPath( nLevel + 1, cFilePath );
			continue;
		}
		
		/* Set default category */
		os::String zCategory = "Other";
		if( cPath.GetLeaf() == "Preferences" )
			zCategory = "Preferences";
		
		
		/* Check if this is an executable and read the category attribute
		   Note: We do not use the registrar to speed things up */
		cFileNode.RewindAttrdir();
		os::String zAttrib;

		bool bExecAttribFound = false;
		bool bCategoryAttribFound = false;
		while( cFileNode.GetNextAttrName( &zAttrib ) == 1 )
		{
			if( zAttrib == "os::MimeType" || zAttrib == "os::Category" )
			{
				char zBuffer[PATH_MAX];
				memset( zBuffer, 0, PATH_MAX );
				if( cFileNode.ReadAttr( zAttrib, ATTR_TYPE_STRING, zBuffer, 0, PATH_MAX ) > 0 )
				{
					if( os::String( zBuffer ) == "application/x-executable" ) {
						bExecAttribFound = true;
					} else {
						zCategory = zBuffer;
						bCategoryAttribFound = true;
					}
				}
			}
		}

		if( !bExecAttribFound && !(	cFileNode.GetMode() & ( S_IXUSR|S_IXGRP|S_IXOTH ) ) || zCategory == "Ignore" )
		{
			cFileNode.Unset();
			continue;
		}
		
		
		cFileNode.Unset();
		
		AddEntry( pcManager, zCategory, cFilePath, zCategory == "Preferences", cPath/*cCurrentDir*/ );
	}
	
	if( pcManager )
		pcManager->Put();
}


