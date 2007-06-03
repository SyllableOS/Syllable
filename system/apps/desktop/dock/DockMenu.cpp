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
#include <util/settings.h>
#include "DockMenu.h"
#include <storage/registrar.h>
#include <cassert>
#include "resources/Dock.h"

DockMenu::DockMenu( os::Handler* pcHandler, os::Rect cFrame, const char* pzName, os::MenuLayout_e eLayout )
	 	: os::Menu( cFrame, pzName, eLayout )
{
	
	
	/* Get the registrar manager */
	os::RegistrarManager* pcManager = NULL;
	try
	{
		pcManager = os::RegistrarManager::Get();
	} catch( ... )
	{
	}
	
	if( pcManager == NULL )
		return;
	
	/* Get application list */
	os::RegistrarAppList cList = pcManager->GetAppList();
	
	/* Add entries */
	for( int i = 0; i < cList.GetCount(); i++ )
		AddEntry( pcManager, cList.GetName( i ), cList.GetCategory( i ), cList.GetPath( i ), 
			os::Path( cList.GetPath( i ) ).GetDir().GetLeaf() == "Preferences",
			os::Path( cList.GetPath( i ) ).GetDir() );
	
	pcManager->Put();
}

DockMenu::~DockMenu()
{
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

void DockMenu::AddEntry( os::RegistrarManager* pcManager, os::String zApplicationName, os::String zCategory, os::Path cPath, bool bUseDirIcon, os::Path cDir )
{
	os::String zTemp;
	os::Image* pcItemIcon = NULL;
	os::Image* pcCategoryIcon = NULL;
	
	//printf( "Add %s %i %s\n", zApplicationName.c_str(), bUseDirIcon, cDir.GetPath().c_str() );
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
		os::String zCategoryIcon = zCategory;	// We need the unlocalized name, so we can get the right icon.
		/* Localizable Category */
		if( zCategory == "Preferences" )
			zCategory = MSG_MENU_CATEGORY_PREFERENCES;
		if( zCategory == "Other" )
			zCategory = MSG_MENU_CATEGORY_OTHER;
		if( zCategory == "Internet" )
			zCategory = MSG_MENU_CATEGORY_INTERNET;
		if( zCategory == "Media" )
			zCategory = MSG_MENU_CATEGORY_MEDIA;
		if( zCategory == "Office" )
			zCategory = MSG_MENU_CATEGORY_OFFICE;
		if( zCategory == "Development" )
			zCategory = MSG_MENU_CATEGORY_DEVELOPMENT;
		if( zCategory == "Games" )
			zCategory = MSG_MENU_CATEGORY_GAMES;
		if( zCategory == "Emulators" )
			zCategory = MSG_MENU_CATEGORY_EMULATORS;
		if( zCategory == "System Tools" )
			zCategory = MSG_MENU_CATEGORY_SYSTEMTOOLS;
			
		/* Create category if necessary */
		os::MenuItem* pcSubMenuItem = FindMenuItem( zCategory );
		if( !pcSubMenuItem )
		{
			if( pcCategoryIcon == NULL )
			{
				try
				{
					os::String cCategoryIcon = os::String( "/system/icons/Dock/" ) + zCategoryIcon + os::String( ".png" );
					os::StreamableIO* pcStream = new os::File( cCategoryIcon );
					os::BitmapImage* pcImg = new os::BitmapImage();
					pcImg->Load( pcStream );
					pcImg->SetSize( os::Point( 24, 24 ) );
					pcCategoryIcon = pcImg;
					delete( pcStream );
				} catch( ... )
				{
					os::File cSelf( open_image_file( get_image_id() ) );
					os::Resources cCol( &cSelf );		
					os::ResStream* pcStream = cCol.GetResourceStream( "folder.png" );
					os::BitmapImage* pcImg = new os::BitmapImage();
					pcImg->Load( pcStream );
					pcImg->SetSize( os::Point( 24, 24 ) );
					pcCategoryIcon = pcImg;
					delete( pcStream );
				}
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
	
	pcAddMenu->AddItem( new os::MenuItem( zApplicationName, pcMsg, "", pcItemIcon ) );
}








