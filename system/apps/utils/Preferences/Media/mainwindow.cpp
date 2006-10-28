
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
#include <util/resources.h>
#include <storage/filereference.h>
#include <storage/directory.h>
#include <storage/fsnode.h>
#include "main.h"
#include "mainwindow.h"
#include "messages.h"
#include "resources/Media.h"

// Settings treeview

SettingsTree::SettingsTree()
			: os::TreeView( os::Rect(), "SettingsTree", os::ListView::F_RENDER_BORDER | 
							os::ListView::F_NO_HEADER | os::ListView::F_NO_AUTO_SORT )
{
}

SettingsTree::~SettingsTree()
{
}

os::Point SettingsTree::GetPreferredSize( bool bLargest ) const
{
	if( !bLargest )
		return( os::Point( 220, 100 ) );
	return( os::Point( 220, 10000 ) );
}


// Preferences view for one input

InputPrefs::InputPrefs( os::MediaInput* pcInput ) : os::VLayoutNode( "Prefs" )
{
	m_pcInput = pcInput;
	m_pcConfigView = m_pcInput->GetConfigurationView();
	if( m_pcConfigView == NULL )
	{
		m_pcConfigView = new os::StringView( os::Rect(), "String", MSG_MAINWND_THISINPUTHASNOCONTROLS );
	}
	AddChild( m_pcConfigView );
	AddChild( new os::VLayoutSpacer( "" ) );

	SetHAlignment( os::ALIGN_LEFT );
	
}
InputPrefs::~InputPrefs()
{
	m_pcInput->Release();
}


// Preferences view for one codec

CodecPrefs::CodecPrefs( os::MediaCodec* pcCodec ) : os::VLayoutNode( "Prefs" )
{
	m_pcCodec = pcCodec;
	
	m_pcConfigView = m_pcCodec->GetConfigurationView();
	if( m_pcConfigView == NULL )
	{
		m_pcConfigView = new os::StringView( os::Rect(), "String", MSG_MAINWND_THISCODECHASNOCONTROLS );
	}
	AddChild( m_pcConfigView );
	AddChild( new os::VLayoutSpacer( "" ) );

	SetHAlignment( os::ALIGN_LEFT );
	
}
CodecPrefs::~CodecPrefs()
{
	m_pcCodec->Release();
}


// Preferences view for one output

OutputPrefs::OutputPrefs( os::MediaOutput* pcOutput ) : os::VLayoutNode( "Prefs" )
{
	m_pcOutput = pcOutput;
	
	m_pcConfigView = m_pcOutput->GetConfigurationView();
	if( m_pcConfigView == NULL )
	{
		m_pcConfigView = new os::StringView( os::Rect(), "String", MSG_MAINWND_THISOUTPUTHASNOCONTROLS );
	}
	AddChild( m_pcConfigView );
	AddChild( new os::VLayoutSpacer( "" ) );

	
	SetHAlignment( os::ALIGN_LEFT );
	
}
OutputPrefs::~OutputPrefs()
{
	m_pcOutput->Release();
}


MainWindow::MainWindow( const os::Rect & cFrame ):os::Window( cFrame, "MainWindow", MSG_MAINWND_TITLE )
{
	os::Rect cBounds = GetBounds();
	os::Rect cRect = os::Rect( 0, 0, 0, 0 );
	
	m_nDeviceSelect = -1;
	
	os::MediaManager::Get();

	// Create the layouts/views
	pcLRoot = new os::LayoutView( cBounds, "L", NULL, os::CF_FOLLOW_ALL );
	pcVLRoot = new os::VLayoutNode( "VL" );
	pcVLRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	

	// Create settings tree
	os::HLayoutNode* pcHDevice = new os::HLayoutNode( "HDevice", 0.0f );
	pcHDevice->SetBorders( os::Rect( 0, 5, 0, 5 ) );
	pcHDevice->AddChild( new os::StringView( os::Rect(), "DeviceLabel", MSG_MAINWND_DEVICE ), 0.0f );
	pcDevice = new os::DropdownMenu( os::Rect(), "Device" );
	pcDevice->SetMinPreferredSize( 18 );
	pcDevice->SetSelectionMessage( new os::Message( M_MW_DEVICE ) );
	pcDevice->SetTarget( this );
	pcDevice->SetReadOnly( true );
	pcHDevice->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHDevice->AddChild( pcDevice );
	pcHDevice->AddChild( new os::HLayoutSpacer( "" ) );
	pcVLRoot->AddChild( pcHDevice );

	// Create settings view
	pcSettingsView = new os::FrameView( os::Rect(), "SettingView", "" );
	pcVLRoot->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	pcVLRoot->AddChild( pcSettingsView );
	
	pcVLRoot->SetHAlignment( os::ALIGN_LEFT );
	
	pcVLRoot->AddChild( new os::Separator( os::Rect(), "", os::HORIZONTAL ), 0.0f );

	// Create default dropdown menus
	os::HLayoutNode* pcHDefaultInput = new os::HLayoutNode( "HDefaultInput", 0.0f );
	pcHDefaultInput->SetBorders( os::Rect( 0, 5, 0, 5 ) );
	pcHDefaultInput->AddChild( new os::StringView( os::Rect(), "DefInputString", MSG_MAINWND_DEFAULT_INPUT ) );
	pcDefaultInput = new os::DropdownMenu( os::Rect(), "DefaultInput" );
	pcDefaultInput->SetMinPreferredSize( 18 );
	pcDefaultInput->SetSelectionMessage( new os::Message( M_MW_INPUT ) );
	pcDefaultInput->SetTarget( this );
	pcDefaultInput->SetReadOnly( true );
	pcHDefaultInput->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHDefaultInput->AddChild( pcDefaultInput );
	pcHDefaultInput->AddChild( new os::HLayoutSpacer( "" ) );
	pcVLRoot->AddChild( pcHDefaultInput );
	
	os::HLayoutNode* pcHDefaultAudioOut = new os::HLayoutNode( "HDefaultAudioOut", 0.0f );
	pcHDefaultAudioOut->SetBorders( os::Rect( 0, 5, 0, 5 ) );
	pcHDefaultAudioOut->AddChild( new os::StringView( os::Rect(), "DefAudioOutString", MSG_MAINWND_DEFAULT_OUTPUT_AUDIO ) );
	pcDefaultAudioOut = new os::DropdownMenu( os::Rect(), "DefaultAudioOut" );
	pcDefaultAudioOut->SetMinPreferredSize( 18 );
	pcDefaultAudioOut->SetSelectionMessage( new os::Message( M_MW_AUDIOOUTPUT ) );
	pcDefaultAudioOut->SetTarget( this );
	pcDefaultAudioOut->SetReadOnly( true );
	pcHDefaultAudioOut->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHDefaultAudioOut->AddChild( pcDefaultAudioOut );
	pcHDefaultAudioOut->AddChild( new os::HLayoutSpacer( "" ) );
	pcVLRoot->AddChild( pcHDefaultAudioOut );

	os::HLayoutNode* pcHDefaultVideoOut = new os::HLayoutNode( "HDefaultVideoOut", 0.0f );
	pcHDefaultVideoOut->SetBorders( os::Rect( 0, 5, 0, 5 ) );
	pcHDefaultVideoOut->AddChild( new os::StringView( os::Rect(), "DefVideoOutString", MSG_MAINWND_DEFAULT_OUTPUT_VIDEO ) );
	pcDefaultVideoOut = new os::DropdownMenu( os::Rect(), "DefaultVideoOut" );
	pcDefaultVideoOut->SetMinPreferredSize( 18 );
	pcDefaultVideoOut->SetSelectionMessage( new os::Message( M_MW_VIDEOOUTPUT ) );
	pcDefaultVideoOut->SetTarget( this );
	pcDefaultVideoOut->SetReadOnly( true );
	pcHDefaultVideoOut->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHDefaultVideoOut->AddChild( pcDefaultVideoOut );
	pcHDefaultAudioOut->AddChild( new os::HLayoutSpacer( "" ) );
	pcVLRoot->AddChild( pcHDefaultVideoOut );
	pcVLRoot->SameWidth( "DefInputString", "DefAudioOutString", "DefVideoOutString", NULL );
	pcVLRoot->SameWidth( "DefaultInput", "DefaultAudioOut", "DefaultVideoOut", NULL );
	
	pcVLRoot->AddChild( new os::Separator( os::Rect(), "", os::HORIZONTAL ), 0.0f );
	
	
	// Create apply/revert/close buttons
	pcHLButtons = new os::HLayoutNode( "HLButtons" );
	pcHLButtons->SetBorders( os::Rect( 0, 5, 0, 0 ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "" ) );
	pcHLButtons->AddChild( pcBControls = new os::Button( cRect, "BControls", MSG_MAINWND_BUTTON_STREAMVOLUMES, new os::Message( M_MW_CONTROLS ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->AddChild( pcBApply = new os::Button( cRect, "BApply", MSG_MAINWND_BUTTON_APPLY, new os::Message( M_MW_APPLY ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->AddChild( pcBUndo = new os::Button( cRect, "BUndo", MSG_MAINWND_BUTTON_UNDO, new os::Message( M_MW_UNDO ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->AddChild( pcBDefault = new os::Button( cRect, "BDefault", MSG_MAINWND_BUTTON_DEFAULT, new os::Message( M_MW_DEFAULT ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->SameWidth( "BControls", "BApply", "BUndo", "BDefault", NULL );
	pcHLButtons->SameHeight( "BControls", "BApply", "BUndo", "BDefault", NULL );
	pcVLRoot->AddChild( pcHLButtons );

	// Set root and add to window
	pcLRoot->SetRoot( pcVLRoot );
	AddChild( pcLRoot );


	// Set default button and initial focus
	this->SetDefaultButton( pcBApply );
	

	// Set tab order
	int iTabOrder = 0;

	pcBDefault->SetTabOrder( iTabOrder++ );
	pcBUndo->SetTabOrder( iTabOrder++ );
	pcBApply->SetTabOrder( iTabOrder++ );
	pcBControls->SetTabOrder( iTabOrder++ );
	pcBApply->MakeFocus();
	

	// Set default outputs 
	os::MediaInput * pcDefaultInput = os::MediaManager::GetInstance()->GetDefaultInput(  );
	os::MediaOutput * pcDefaultVideo = os::MediaManager::GetInstance()->GetDefaultVideoOutput(  );
	os::MediaOutput * pcDefaultAudio = os::MediaManager::GetInstance()->GetDefaultAudioOutput(  );

	if ( pcDefaultInput )
	{
		cCurrentInput = pcDefaultInput->GetIdentifier();
		pcDefaultInput->Release();
	}

	if ( pcDefaultAudio )
	{
		cCurrentAudio = pcDefaultAudio->GetIdentifier();
		pcDefaultAudio->Release();
	}

	if ( pcDefaultVideo )
	{
		cCurrentVideo = pcDefaultVideo->GetIdentifier();
		pcDefaultVideo->Release();
	}

	
	cUndoInput = cCurrentInput;
	cUndoVideo = cCurrentVideo;
	cUndoAudio = cCurrentAudio;
	
	// Set Icon
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );

	// Show data
	ShowData();
	
	// Select first item */
	if( pcDevice->GetItemCount() > 0 )
		pcDevice->SetSelection( 0 );
}

MainWindow::~MainWindow()
{
	os::LayoutNode* pcOld = pcSettingsView->GetRoot();
	pcSettingsView->SetRoot( NULL );
	delete( pcOld );
	os::MediaManager::GetInstance()->Put();
}

void MainWindow::ShowData()
{
	// Default dropdownmenu entries 
	pcDevice->Clear();
	pcDefaultInput->Clear();
	pcDefaultAudioOut->Clear();
	pcDefaultVideoOut->Clear();
	
	// Add inputs
	os::MediaInput* pcInput;
	uint32 nIndex = 0;
	m_nInputStart = 0;

	while ( ( pcInput = os::MediaManager::GetInstance()->GetInput( nIndex ) ) != NULL )
	{
		pcDevice->AppendItem( pcInput->GetIdentifier() );
		
		if( !pcInput->FileNameRequired() )
		{
			pcDefaultInput->AppendItem( pcInput->GetIdentifier() );
			if( pcInput->GetIdentifier() == cCurrentInput )
				pcDefaultInput->SetSelection( pcDefaultInput->GetItemCount() - 1, false );
		}
		pcInput->Release();
		nIndex++;
	}
	
	// Add codecs
	os::MediaCodec* pcCodec;
	m_nCodecStart = m_nInputStart + nIndex;
	nIndex = 0;

	while ( ( pcCodec = os::MediaManager::GetInstance()->GetCodec( nIndex ) ) != NULL )
	{
		pcDevice->AppendItem( pcCodec->GetIdentifier() );
		pcCodec->Release();
		nIndex++;
	}
	
	// Add outputs
	os::MediaOutput * pcOutput;
	m_nOutputStart = m_nCodecStart + nIndex;
	nIndex = 0;

	while ( ( pcOutput = os::MediaManager::GetInstance()->GetOutput( nIndex ) ) != NULL )
	{
		if( !pcOutput->FileNameRequired() )
		{
			// Add to default dropdown menus 
			for( int i = 0; i < pcOutput->GetOutputFormatCount(); i++ )
			{
				os::MediaFormat_s sFormat = pcOutput->GetOutputFormat( i );
				if( sFormat.nType == os::MEDIA_TYPE_AUDIO ) {
					pcDefaultAudioOut->AppendItem( pcOutput->GetIdentifier() );
					if( pcOutput->GetIdentifier() == cCurrentAudio )
						pcDefaultAudioOut->SetSelection( pcDefaultAudioOut->GetItemCount() - 1, false );
					break;
				}
			}
			for( int i = 0; i < pcOutput->GetOutputFormatCount(); i++ )
			{
				os::MediaFormat_s sFormat = pcOutput->GetOutputFormat( i );
				if( sFormat.nType == os::MEDIA_TYPE_VIDEO ) {
					pcDefaultVideoOut->AppendItem( pcOutput->GetIdentifier() );
					if( pcOutput->GetIdentifier() == cCurrentVideo )
						pcDefaultVideoOut->SetSelection( pcDefaultVideoOut->GetItemCount() - 1, false );

					break;
				}
			}
		}
		pcDevice->AppendItem( pcOutput->GetIdentifier() );
		
		pcOutput->Release();
		nIndex++;
	}
	
	if( m_nDeviceSelect > -1 )
		pcDevice->SetSelection( m_nDeviceSelect, false );

	Flush();
}

void MainWindow::Apply()
{
	os::MediaManager::GetInstance()->SetDefaultInput( cCurrentInput );
	os::MediaManager::GetInstance()->SetDefaultAudioOutput( cCurrentAudio );
	os::MediaManager::GetInstance()->SetDefaultVideoOutput( cCurrentVideo );
}

void MainWindow::Undo()
{
	// Copy original state back
	cCurrentInput = cUndoInput;
	cCurrentVideo = cUndoVideo;
	cCurrentAudio = cUndoAudio;
	
	// Update controls
	ShowData();
}

void MainWindow::Default()
{
	// TODO: What to do here ?
	// Copy original state back
	cCurrentInput = cUndoInput;
	cCurrentVideo = cUndoVideo;
	cCurrentAudio = cUndoAudio;

	// Update controls
	ShowData();
}

void MainWindow::DeviceChanged()
{
	/* Look what has been selected */
	int nSelected = pcDevice->GetSelection();
	m_nDeviceSelect = nSelected;

	if( nSelected >= m_nInputStart && nSelected < m_nCodecStart ) {
		os::MediaInput* pcInput = os::MediaManager::GetInstance()->GetInput( nSelected - m_nInputStart );
		
		os::LayoutNode* pcOld = pcSettingsView->GetRoot();
		pcSettingsView->SetRoot( new InputPrefs( pcInput ) );
		delete( pcOld );
		pcLRoot->InvalidateLayout();
	} else if( nSelected >= m_nCodecStart && nSelected < m_nOutputStart ) {
		os::MediaCodec* pcCodec = os::MediaManager::GetInstance()->GetCodec( nSelected - m_nCodecStart );
		os::LayoutNode* pcOld = pcSettingsView->GetRoot();
		pcSettingsView->SetRoot( new CodecPrefs( pcCodec ) );
		delete( pcOld );
		pcLRoot->InvalidateLayout();
	} else if( nSelected >= m_nOutputStart && nSelected < (int)pcDevice->GetItemCount() ) {
		os::MediaOutput* pcOutput = os::MediaManager::GetInstance()->GetOutput( nSelected - m_nOutputStart );
		os::LayoutNode* pcOld = pcSettingsView->GetRoot();
		pcSettingsView->SetRoot( new OutputPrefs( pcOutput ) );
		delete( pcOld );
		pcLRoot->InvalidateLayout();
	} else {
		pcLRoot->InvalidateLayout();
	}
	pcSettingsView->Invalidate();
	Flush();
}

void MainWindow::InputChange()
{
	cCurrentInput = pcDefaultInput->GetCurrentString();
}

void MainWindow::VideoOutputChange()
{
	cCurrentVideo = pcDefaultVideoOut->GetCurrentString();
}


void MainWindow::AudioOutputChange()
{
	cCurrentAudio = pcDefaultAudioOut->GetCurrentString();
}

void MainWindow::HandleMessage( os::Message * pcMessage )
{
	// Get message code and act on it
	switch ( pcMessage->GetCode() )
	{
	case M_MW_INPUT:
		InputChange();
		break;
	case M_MW_VIDEOOUTPUT:
		VideoOutputChange();
		break;
	case M_MW_AUDIOOUTPUT:
		AudioOutputChange();
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

	case M_MW_DEVICE:
		DeviceChanged();
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

