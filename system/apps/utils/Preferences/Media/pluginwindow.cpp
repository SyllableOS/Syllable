
/*  Syllable Media Preferences
 *  Copyright (C) 2003 Arno Klenke
 *  Based on the preferences code by Daryl Dudey
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  Generalr Public License as published by the Free Software
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

#include <stdio.h>
#include <dirent.h>
#include <util/application.h>
#include <util/message.h>
#include <gui/desktop.h>
#include <gui/guidefines.h>
#include <storage/filereference.h>
#include <storage/directory.h>
#include <storage/fsnode.h>
#include "main.h"
#include "mainwindow.h"
#include "pluginwindow.h"
#include "messages.h"

PluginWindow::PluginWindow( const os::Rect & cFrame ):os::Window( cFrame, "PluginWindow", "Media Plugins", os::WND_NOT_RESIZABLE )
{
	m_pcPluginList = new os::ListView( os::Rect( 0, 0, 150, cFrame.Height() ), "plugin_list", os::ListView::F_RENDER_BORDER | os::ListView::F_NO_AUTO_SORT, os::CF_FOLLOW_LEFT | os::CF_FOLLOW_TOP | os::CF_FOLLOW_BOTTOM );
	m_pcPluginList->SetSelChangeMsg( new os::Message( M_MW_PLUGIN ) );
	m_pcPluginList->InsertColumn( "Plugin", 147 );
	m_pcPluginList->InsertColumn( "", 0 );
	m_pcPluginList->SetTarget( this );
	AddChild( m_pcPluginList );


	m_pcConfigView = NULL;
	// Show data
	ShowData();
}

PluginWindow::~PluginWindow()
{
}

void PluginWindow::ShowData()
{
	os::MediaManager * pcManager = os::MediaManager::GetInstance();
	if ( !pcManager )
		return;

	uint32 i = 0;

	nInputStart = nOutputStart = 0;
	os::MediaInput * pcInput;
	while ( ( pcInput = pcManager->GetInput( i ) ) != NULL )
	{
		os::ListViewStringRow * pcRow = new os::ListViewStringRow();
		pcRow->AppendString( pcInput->GetIdentifier() );
		pcRow->AppendString( "" );
		m_pcPluginList->InsertRow( pcRow );
		nOutputStart++;
		delete( pcInput );
		i++;
	}

	i = 0;
	nCodecStart = nOutputStart;
	os::MediaOutput * pcOutput;
	while ( ( pcOutput = pcManager->GetOutput( i ) ) != NULL )
	{
		os::ListViewStringRow * pcRow = new os::ListViewStringRow();
		pcRow->AppendString( pcOutput->GetIdentifier() );
		pcRow->AppendString( "" );
		m_pcPluginList->InsertRow( pcRow );
		nCodecStart++;
		delete( pcOutput );
		i++;
	}

	i = 0;
	os::MediaCodec * pcCodec;
	while ( ( pcCodec = pcManager->GetCodec( i ) ) != NULL )
	{
		os::ListViewStringRow * pcRow = new os::ListViewStringRow();
		pcRow->AppendString( pcCodec->GetIdentifier() );
		pcRow->AppendString( "" );
		m_pcPluginList->InsertRow( pcRow );
		delete( pcCodec );
		i++;
	}
	m_pcPluginList->Select( 0 );
}

void PluginWindow::Close()
{
	PostMessage( os::M_QUIT );
}

void PluginWindow::PluginChange()
{
	os::MediaManager * pcManager = os::MediaManager::GetInstance();
	if ( m_pcConfigView )
	{
		RemoveChild( m_pcConfigView );
		delete( m_pcConfigView );
	}
	m_pcConfigView = NULL;

	if ( ( uint32 )m_pcPluginList->GetFirstSelected() >= nCodecStart )
	{
		os::MediaCodec * pcCodec;
		if ( ( pcCodec = pcManager->GetCodec( ( uint32 )m_pcPluginList->GetFirstSelected() - nCodecStart ) ) != NULL )
		{
			m_pcConfigView = pcCodec->GetConfigurationView();
			delete( pcCodec );
		}
	}
	else if ( ( uint32 )m_pcPluginList->GetFirstSelected() >= nOutputStart )
	{
		os::MediaOutput * pcOutput;
		if ( ( pcOutput = pcManager->GetOutput( ( uint32 )m_pcPluginList->GetFirstSelected() - nOutputStart ) ) != NULL )
		{
			m_pcConfigView = pcOutput->GetConfigurationView();
			delete( pcOutput );
		}
	}
	if ( m_pcConfigView != NULL )
	{
		m_pcConfigView->MoveBy( os::Point( 150, 0 ) );
		os::Rect cFrame = GetFrame();
		cFrame.right = cFrame.left + 150 + m_pcConfigView->GetBounds().Width(  );
		cFrame.bottom = cFrame.top + m_pcConfigView->GetBounds().Height(  );
		SetFrame( cFrame );
		AddChild( m_pcConfigView );
	}
	else
	{
		os::Rect cFrame = GetFrame();
		cFrame.right = cFrame.left + 150;
		cFrame.bottom = cFrame.top + 250;
		SetFrame( cFrame );
	}
}

void PluginWindow::HandleMessage( os::Message * pcMessage )
{
	// Get message code and act on it
	switch ( pcMessage->GetCode() )
	{

	case M_MW_PLUGIN:
		PluginChange();
		break;

	case M_MW_CLOSE:
		Close();
		break;

	default:
		os::Window::HandleMessage( pcMessage );
		break;
	}

}

bool PluginWindow::OkToQuit()
{
	return true;
}
