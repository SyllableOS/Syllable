
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

MainWindow::MainWindow( const os::Rect & cFrame ):os::Window( cFrame, "MainWindow", "Media", os::WND_NOT_RESIZABLE )
{
	os::Rect cBounds = GetBounds();
	os::Rect cRect = os::Rect( 0, 0, 0, 0 );

	// Create the layouts/views
	pcLRoot = new os::LayoutView( cBounds, "L", NULL, os::CF_FOLLOW_ALL );
	pcVLRoot = new os::VLayoutNode( "VL" );
	pcVLRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );

	// Store v-node to store output settings
	pcVLSettings = new os::VLayoutNode( "VLSettings" );
	pcVLSettings->SetBorders( os::Rect( 5, 5, 5, 5 ) );

	// Store v-node to store sound settings
	pcVLSounds = new os::VLayoutNode( "VLSounds" );
	pcVLSounds->SetBorders( os::Rect( 5, 5, 5, 5 ) );

	// Video Output
	pcHLVideoOutput = new os::HLayoutNode( "HLVideoOutput" );
	pcHLVideoOutput->AddChild( new os::StringView( cRect, "SVVideoOutput", "Video" ) );
	pcHLVideoOutput->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLVideoOutput->AddChild( pcDDMVideoOutput = new os::DropdownMenu( cRect, "DDMVideoOutput" ) );
	pcDDMVideoOutput->SetMinPreferredSize( 22 );
	pcDDMVideoOutput->SetMaxPreferredSize( 22 );
	pcDDMVideoOutput->SetReadOnly( true );
	pcDDMVideoOutput->SetSelectionMessage( new os::Message( M_MW_VIDEOOUTPUT ) );
	pcDDMVideoOutput->SetTarget( this );
	pcVLSettings->AddChild( pcHLVideoOutput );
	pcVLSettings->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );

	// Audio Output
	pcHLAudioOutput = new os::HLayoutNode( "HLAudioOutput" );
	pcHLAudioOutput->AddChild( new os::StringView( cRect, "SVAudioOutput", "Audio" ) );
	pcHLAudioOutput->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLAudioOutput->AddChild( pcDDMAudioOutput = new os::DropdownMenu( cRect, "DDMAudioOutput" ) );
	pcDDMAudioOutput->SetMinPreferredSize( 22 );
	pcDDMAudioOutput->SetMaxPreferredSize( 22 );
	pcDDMAudioOutput->SetReadOnly( true );
	pcDDMAudioOutput->SetSelectionMessage( new os::Message( M_MW_AUDIOOUTPUT ) );
	pcDDMAudioOutput->SetTarget( this );
	pcVLSettings->AddChild( pcHLAudioOutput );
	pcVLSettings->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );

	// Default DSP
	pcHLDefaultDSP = new os::HLayoutNode( "HLDefaultDsp" );
	pcHLDefaultDSP->AddChild( new os::StringView( cRect, "SVDefaultDSP", "Default DSP" ) );
	pcHLDefaultDSP->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLDefaultDSP->AddChild( pcDDMDefaultDsp = new os::DropdownMenu( cRect, "DDMDefaultDSP" ) );
	pcDDMDefaultDsp->SetMinPreferredSize( 22 );
	pcDDMDefaultDsp->SetMaxPreferredSize( 22 );
	pcDDMDefaultDsp->SetReadOnly( true );
	pcDDMDefaultDsp->SetSelectionMessage( new os::Message( M_MW_DEFAULTDSP ) );
	pcDDMDefaultDsp->SetTarget( this );
	pcVLSettings->AddChild( pcHLDefaultDSP );
	pcVLSettings->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );

	// Startup Sound
	pcHLStartupSound = new os::HLayoutNode( "HLStartupSound" );
	pcHLStartupSound->AddChild( new os::StringView( cRect, "SVStartupSound", "Start" ) );
	pcHLStartupSound->AddChild( new os::HLayoutSpacer( "", 45.0f, 45.0f ) );
	pcHLStartupSound->AddChild( pcDDMStartupSound = new os::DropdownMenu( cRect, "DDMStartupSound" ) );
	pcDDMStartupSound->SetMinPreferredSize( 22 );
	pcDDMStartupSound->SetMaxPreferredSize( 22 );
	pcDDMStartupSound->SetReadOnly( true );
	pcDDMStartupSound->SetSelectionMessage( new os::Message( M_MW_STARTUPSOUND ) );
	pcDDMStartupSound->SetTarget( this );
	pcVLSounds->AddChild( pcHLStartupSound );
	pcVLSounds->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );

	// Align the controls
	pcVLSettings->SameWidth( "SVVideoOutput", "SVAudioOutput", "SVDefaultDSP", NULL );
	pcVLSettings->SameWidth( "DDMVideoOutput", "DDMAudioOutput", "DDMDefaultDSP", NULL );
	pcVLSounds->SameWidth( "SVStartupSound", NULL );
	pcVLSounds->SameWidth( "DDMStartupSound", NULL );

	// Create frameview to store output settings
	pcFVSettings = new os::FrameView( cBounds, "FVSettings", "Outputs", os::CF_FOLLOW_ALL );
	pcFVSettings->SetRoot( pcVLSettings );
	pcVLRoot->AddChild( pcFVSettings );

	// Create frameview to store sound settings
	pcFVSounds = new os::FrameView( cBounds, "FVSounds", "Sounds", os::CF_FOLLOW_ALL );
	pcFVSounds->SetRoot( pcVLSounds );
	pcVLRoot->AddChild( pcFVSounds );

	pcVLRoot->SameWidth( "FVSettings", "FVSounds", NULL );

	// Create apply/revert/close buttons
	pcHLButtons = new os::HLayoutNode( "HLButtons" );
	pcVLRoot->AddChild( new os::VLayoutSpacer( "", 10.0f, 10.0f ) );
	pcHLButtons = new os::HLayoutNode( "HLButtons" );
	pcHLButtons->AddChild( pcBPlugins = new os::Button( cRect, "BPlugins", "Plugins...", new os::Message( M_MW_PLUGIN ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "" ) );
	pcHLButtons->AddChild( pcBControls = new os::Button( cRect, "BControls", "Controls...", new os::Message( M_MW_CONTROLS ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "" ) );
	pcHLButtons->AddChild( pcBApply = new os::Button( cRect, "BApply", "Apply", new os::Message( M_MW_APPLY ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->AddChild( pcBUndo = new os::Button( cRect, "BUndo", "Undo", new os::Message( M_MW_UNDO ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->AddChild( pcBDefault = new os::Button( cRect, "BDefault", "Default", new os::Message( M_MW_DEFAULT ) ) );
	pcHLButtons->SameWidth( "BApply", "BUndo", "BDefault", NULL );
	pcVLRoot->AddChild( pcHLButtons );

	// Set root and add to window
	pcLRoot->SetRoot( pcVLRoot );
	AddChild( pcLRoot );

	// Set default button and initial focus
	this->SetDefaultButton( pcBApply );
	pcDDMVideoOutput->MakeFocus();



	// Set tab order
	int iTabOrder = 0;

	pcDDMVideoOutput->SetTabOrder( iTabOrder++ );
	pcDDMAudioOutput->SetTabOrder( iTabOrder++ );
	pcDDMDefaultDsp->SetTabOrder( iTabOrder++ );
	pcDDMStartupSound->SetTabOrder( iTabOrder++ );
	pcBDefault->SetTabOrder( iTabOrder++ );
	pcBUndo->SetTabOrder( iTabOrder++ );
	pcBApply->SetTabOrder( iTabOrder++ );
	pcBPlugins->SetTabOrder( iTabOrder++ );


	// Set default outputs 
	os::MediaOutput * pcDefaultVideo = os::MediaManager::GetInstance()->GetDefaultVideoOutput(  );
	os::MediaOutput * pcDefaultAudio = os::MediaManager::GetInstance()->GetDefaultAudioOutput(  );

	if ( pcDefaultAudio )
	{
		cCurrentAudio = pcDefaultAudio->GetIdentifier();
		delete( pcDefaultAudio );
	}

	if ( pcDefaultVideo )
	{
		cCurrentVideo = pcDefaultVideo->GetIdentifier();
		delete( pcDefaultVideo );
	}

	// Default DSP
	os::Message cReply;
	os::MediaManager::GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_GET_DSP_COUNT, &cReply );
	int32 nCount, hDefault;

	if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindInt32( "dsp_count", &nCount ) == 0 )
	{
		nDspCount = nCount;

		os::MediaManager::GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_GET_DEFAULT_DSP, &cReply );
		if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindInt32( "handle", &hDefault ) == 0 )
		{
			hCurrentDsp = hDefault;
		}
		else
			hCurrentDsp = 0;
	}
	else	// No DSP's have been found by the server
	{
		nDspCount = 0;
		hCurrentDsp = 0;
		pcDDMDefaultDsp->SetEnable( false );
	}

	// Startup sound
	os::MediaManager::GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_GET_STARTUP_SOUND, &cReply );
	os::String zTemp;

	if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindString( "sound", &zTemp.str(  ) ) == 0 )
		cCurrentStartupSound = zTemp;
	else
		cCurrentStartupSound = "None";


	cUndoVideo = cCurrentVideo;
	cUndoAudio = cCurrentAudio;
	hUndoDsp = hCurrentDsp;
	cUndoStartupSound = cCurrentStartupSound;

	// Show data
	ShowData();
}

MainWindow::~MainWindow()
{
}

void MainWindow::ShowData()
{
	// Look for all outputs which do not require a filename
	os::MediaOutput * pcOutput;
	os::MediaFormat_s sFormat;
	uint32 nIndex = 0;

	pcDDMVideoOutput->Clear();
	pcDDMAudioOutput->Clear();
	pcDDMDefaultDsp->Clear();
	while ( ( pcOutput = os::MediaManager::GetInstance()->GetOutput( nIndex ) ) != NULL )
	{
		if ( !pcOutput->FileNameRequired() )
		{
			bool bAudio = false;
			bool bVideo = false;

			for ( uint32 i = 0; i < pcOutput->GetOutputFormatCount(); i++ )
			{
				sFormat = pcOutput->GetOutputFormat( i );
				if ( sFormat.nType == os::MEDIA_TYPE_VIDEO )
					// Video output
					bVideo = true;
				if ( sFormat.nType == os::MEDIA_TYPE_AUDIO )
					// Audio output
					bAudio = true;
			}
			if ( bVideo )
			{
				pcDDMVideoOutput->AppendItem( pcOutput->GetIdentifier().c_str(  ) );
				if ( pcOutput->GetIdentifier() == cCurrentVideo )
					pcDDMVideoOutput->SetSelection( pcDDMVideoOutput->GetItemCount() - 1, false );
			}
			if ( bAudio )
			{
				pcDDMAudioOutput->AppendItem( pcOutput->GetIdentifier().c_str(  ) );
				if ( pcOutput->GetIdentifier() == cCurrentAudio )
					pcDDMAudioOutput->SetSelection( pcDDMAudioOutput->GetItemCount() - 1, false );
			}

		}
		delete( pcOutput );
		nIndex++;
	}
	// Fill in DSP entries
	if( nDspCount > 0 )
	{
		std::string cName;

		for( int32 j=0; j < nDspCount; j++ )
		{
			os::Message cReply;
			os::Message cMsg( os::MEDIA_SERVER_GET_DSP_INFO );
			cMsg.AddInt32( "handle", j );
			os::MediaManager::GetInstance()->GetServerLink().SendMessage( &cMsg, &cReply );

			if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindString( "name", &cName ) == 0 )
				pcDDMDefaultDsp->AppendItem( cName.c_str() );
		}
	}

	if ( pcDDMVideoOutput->GetItemCount() == 0 )
		pcDDMVideoOutput->SetEnable( false );
	if ( pcDDMAudioOutput->GetItemCount() == 0 )
		pcDDMAudioOutput->SetEnable( false );
	if ( pcDDMDefaultDsp->GetItemCount() == 0 )
		pcDDMDefaultDsp->SetEnable( false );
	else
		pcDDMDefaultDsp->SetSelection( hCurrentDsp, false );

	// Add Sounds
	pcDDMStartupSound->Clear();
	pcDDMStartupSound->AppendItem( "None" );
	pcDDMStartupSound->SetSelection( 0, false );

	os::FSNode cDirNode = os::FSNode( "/system/sounds" );
	if ( cDirNode.IsValid() )
	{
		os::Directory * pcDir = new os::Directory( cDirNode );
		os::FileReference cFileRef;
		// Read all files of the /system/sounds directory
		pcDir->Rewind();
		while ( pcDir->GetNextEntry( &cFileRef ) == 1 )
		{
			if ( cFileRef.GetName() == ".." || cFileRef.GetName(  ) == "." )
				continue;
			pcDDMStartupSound->AppendItem( cFileRef.GetName().c_str(  ) );
			if ( cFileRef.GetName() == cCurrentStartupSound.str(  ) )
				pcDDMStartupSound->SetSelection( pcDDMStartupSound->GetItemCount() - 1, false );
		}
		delete( pcDir );
	}

}

void MainWindow::Apply()
{
	os::MediaManager::GetInstance()->SetDefaultAudioOutput( cCurrentAudio );
	os::MediaManager::GetInstance()->SetDefaultVideoOutput( cCurrentVideo );
	os::Message cSoundMsg( os::MEDIA_SERVER_SET_STARTUP_SOUND );
	cSoundMsg.AddString( "sound", cCurrentStartupSound );
	os::MediaManager::GetInstance()->GetServerLink(  ).SendMessage( &cSoundMsg );
	os::Message cDspMsg( os::MEDIA_SERVER_SET_DEFAULT_DSP );
	cDspMsg.AddInt32( "handle", hCurrentDsp );
	os::MediaManager::GetInstance()->GetServerLink(  ).SendMessage( &cDspMsg );
}

void MainWindow::Undo()
{
	// Copy original state back
	cCurrentVideo = cUndoVideo;
	cCurrentAudio = cUndoAudio;
	cCurrentStartupSound = cUndoStartupSound;
	hCurrentDsp = hUndoDsp;

	// Update controls
	ShowData();
}

void MainWindow::Default()
{
	// TODO: What to do here ?
	// Copy original state back
	cCurrentVideo = cUndoVideo;
	cCurrentAudio = cUndoAudio;
	cCurrentStartupSound = cUndoStartupSound;
	hCurrentDsp = 0;

	// Update controls
	ShowData();
}

void MainWindow::Plugins()
{
	PluginWindow *pcWin = new PluginWindow( os::Rect( GetFrame().left + 50, GetFrame(  ).top + 50,
			GetFrame().left + 200, GetFrame(  ).top + 300 ) );

	pcWin->Show();
	pcWin->MakeFocus( true );
}

void MainWindow::OutputChange()
{
	cCurrentVideo = pcDDMVideoOutput->GetItem( pcDDMVideoOutput->GetSelection() );
	cCurrentAudio = pcDDMAudioOutput->GetItem( pcDDMAudioOutput->GetSelection() );
	cCurrentStartupSound = pcDDMStartupSound->GetItem( pcDDMStartupSound->GetSelection() );
}

void MainWindow::DspChange()
{
	hCurrentDsp = pcDDMDefaultDsp->GetSelection();
}

void MainWindow::HandleMessage( os::Message * pcMessage )
{
	// Get message code and act on it
	switch ( pcMessage->GetCode() )
	{

	case M_MW_VIDEOOUTPUT:
	case M_MW_AUDIOOUTPUT:
	case M_MW_STARTUPSOUND:
		OutputChange();
		break;

	case M_MW_DEFAULTDSP:
		DspChange();
		break;

	case M_MW_APPLY:
		Apply();
		break;

	case M_MW_DEFAULT:
		Default();
		break;

	case M_MW_UNDO:
		Undo();
		break;

	case M_MW_PLUGIN:
		Plugins();
		break;
	
	case M_MW_CONTROLS:
		os::MediaManager::GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_SHOW_CONTROLS );
		break;

	default:
		os::Window::HandleMessage( pcMessage );
		break;
	}

}

bool MainWindow::OkToQuit()
{
	os::Application::GetInstance()->PostMessage( os::M_QUIT );
	return true;
}








