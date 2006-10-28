/*  Syllable FileBrowser
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
 
#include <iostream>
#include "application.h"
#include "mainwindow.h"

App::App( os::String zPath ) : os::Application( "application/x-vnd.syllable-FileBrowser" )
{
	SetCatalog("FileBrowser.catalog");
	m_pcMainWindow = new MainWindow( zPath );
	m_pcMainWindow->Show();
	m_pcMainWindow->MakeFocus();
}

App::~App()
{
}

bool App::OkToQuit()
{
	return( true );
}










