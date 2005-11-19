
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
	return( os::Point( 200, 10000 ) );
}


// Preferences view for one input

InputPrefs::InputPrefs( os::MediaInput* pcInput ) : os::VLayoutNode( "Prefs" )
{
	m_pcInput = pcInput;
	
	m_pcConfigView = m_pcInput->GetConfigurationView();
	if( m_pcConfigView == NULL )
	{
		m_pcConfigView = new os::StringView( os::Rect(), "String", "This input has no controls" ); 
	}
	AddChild( m_pcConfigView );
	AddChild( new os::VLayoutSpacer( "" ) );

	SetHAlignment( os::ALIGN_LEFT );
	
}
InputPrefs::~InputPrefs()
{
	delete( m_pcInput );
}


// Preferences view for one codec

CodecPrefs::CodecPrefs( os::MediaCodec* pcCodec ) : os::VLayoutNode( "Prefs" )
{
	m_pcCodec = pcCodec;
	
	m_pcConfigView = m_pcCodec->GetConfigurationView();
	if( m_pcConfigView == NULL )
	{
		m_pcConfigView = new os::StringView( os::Rect(), "String", "This codec has no controls" ); 
	}
	AddChild( m_pcConfigView );
	AddChild( new os::VLayoutSpacer( "" ) );

	SetHAlignment( os::ALIGN_LEFT );
	
}
CodecPrefs::~CodecPrefs()
{
	delete( m_pcCodec );
}


// Preferences view for one output

OutputPrefs::OutputPrefs( os::MediaOutput* pcOutput, bool bDefaultVideo, bool bDefaultAudio ) : os::VLayoutNode( "Prefs" )
{
	m_pcOutput = pcOutput;
	
	m_pcConfigView = m_pcOutput->GetConfigurationView();
	if( m_pcConfigView == NULL )
	{
		m_pcConfigView = new os::StringView( os::Rect(), "String", "This output has no controls" ); 
	}
	AddChild( m_pcConfigView );
	AddChild( new os::VLayoutSpacer( "" ) );

	m_pcDefaultVideo = new os::CheckBox( os::Rect(), "DefaultVideo", "Default video output", new os::Message( M_MW_VIDEOOUTPUT ) );
	m_pcDefaultAudio = new os::CheckBox( os::Rect(), "DefaultAudio", "Default audio output", new os::Message( M_MW_AUDIOOUTPUT ) );
	
	m_pcDefaultVideo->SetEnable( false );
	m_pcDefaultAudio->SetEnable( false );
	
	// Set value
	m_pcDefaultVideo->SetValue( bDefaultVideo );
	m_pcDefaultAudio->SetValue( bDefaultAudio );
	
	// Enable checkboxes if possible
	if( !m_pcOutput->FileNameRequired() )
	for( uint32 i = 0; i < m_pcOutput->GetOutputFormatCount(); i++ )
	{
		if( m_pcOutput->GetOutputFormat( i ).nType == os::MEDIA_TYPE_VIDEO )
			m_pcDefaultVideo->SetEnable( true );
		if( m_pcOutput->GetOutputFormat( i ).nType == os::MEDIA_TYPE_AUDIO )
			m_pcDefaultAudio->SetEnable( true );
	}
	
	
	// Disable chekboxes if they are marked
	if( bDefaultVideo )
		m_pcDefaultVideo->SetEnable( false );	
	if( bDefaultAudio )
		m_pcDefaultAudio->SetEnable( false );
	
	
	AddChild( m_pcDefaultVideo );
	AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	AddChild( m_pcDefaultAudio );
	AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	
	SetHAlignment( os::ALIGN_LEFT );
	
	SameWidth( "DefaultVideo", "DefaultAudio", NULL );
	SameHeight( "DefaultVideo", "DefaultAudio", NULL );
}
OutputPrefs::~OutputPrefs()
{
	delete( m_pcOutput );
}

// Preferences view for sounds

SoundPrefs::SoundPrefs( os::Window* pcParent, os::String zCurrentStartup ) : os::VLayoutNode( "Prefs" )
{
	// Add Sounds
	m_pcStartupSound = new os::DropdownMenu( os::Rect(), "StartupSound" );
	m_pcStartupSound->SetMinPreferredSize( 22 );
	m_pcStartupSound->SetMaxPreferredSize( 22 );
	m_pcStartupSound->SetReadOnly( true );
	m_pcStartupSound->SetSelectionMessage( new os::Message( M_MW_STARTUPSOUND ) );
	m_pcStartupSound->SetTarget( pcParent );
	
	m_pcStartupSound->Clear();
	m_pcStartupSound->AppendItem( "None" );
	m_pcStartupSound->SetSelection( 0, false );

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
			m_pcStartupSound->AppendItem( cFileRef.GetName().c_str(  ) );
			if ( cFileRef.GetName() == zCurrentStartup.str(  ) )
				m_pcStartupSound->SetSelection( m_pcStartupSound->GetItemCount() - 1, false );
		}
		delete( pcDir );
	}
	
	AddChild( m_pcStartupSound );
	AddChild( new os::VLayoutSpacer( "" ) );

	SetHAlignment( os::ALIGN_LEFT );
	
}
SoundPrefs::~SoundPrefs()
{
	RemoveChild( m_pcStartupSound );
	delete( m_pcStartupSound );
}


MainWindow::MainWindow( const os::Rect & cFrame ):os::Window( cFrame, "MainWindow", "Media" )
{
	os::Rect cBounds = GetBounds();
	os::Rect cRect = os::Rect( 0, 0, 0, 0 );
	
	m_nTreeSelect = -1;

	// Create the layouts/views
	pcLRoot = new os::LayoutView( cBounds, "L", NULL, os::CF_FOLLOW_ALL );
	pcVLRoot = new os::VLayoutNode( "VL" );
	pcVLRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	
	
	// Store v-node to store output settings
	pcHLSettings = new os::HLayoutNode( "HLSettings" );
	pcHLSettings->SetBorders( os::Rect( 5, 5, 5, 5 ) );

	
	// Create settings tree
	m_pcTree = new SettingsTree();
	m_pcTree->InsertColumn( "", (int)cBounds.Width() );
	m_pcTree->SetSelChangeMsg( new os::Message( M_MW_TREEVIEW ) );
	
	// Create prefs node
	m_pcPrefs = NULL;
 
	// Add everything
	pcHLSettings->AddChild( m_pcTree );
	pcHLSettings->AddChild( new os::HLayoutSpacer( "", 10.0f, 10.0f ) );
	pcVLRoot->AddChild( pcHLSettings );
	
	pcVLRoot->SetHAlignment( os::ALIGN_LEFT );
	
	

	// Create apply/revert/close buttons
	pcHLButtons = new os::HLayoutNode( "HLButtons" );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "" ) );
	pcHLButtons->AddChild( pcBControls = new os::Button( cRect, "BControls", "Controls...", new os::Message( M_MW_CONTROLS ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->AddChild( pcBApply = new os::Button( cRect, "BApply", "Apply", new os::Message( M_MW_APPLY ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->AddChild( pcBUndo = new os::Button( cRect, "BUndo", "Undo", new os::Message( M_MW_UNDO ) ) );
	pcHLButtons->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	pcHLButtons->AddChild( pcBDefault = new os::Button( cRect, "BDefault", "Default", new os::Message( M_MW_DEFAULT ) ) );
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

	os::Message cReply;
	os::MediaManager::GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_GET_STARTUP_SOUND, &cReply );
	os::String zTemp;

	if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindString( "sound", &zTemp.str(  ) ) == 0 )
		cCurrentStartupSound = zTemp;
	else
		cCurrentStartupSound = "None";


	cUndoVideo = cCurrentVideo;
	cUndoAudio = cCurrentAudio;
	cUndoStartupSound = cCurrentStartupSound;
	
	// Set Icon
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon24x24.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );

	// Show data
	ShowData();
}

MainWindow::~MainWindow()
{
	// Clear treeview
	m_pcTree->Clear();
}

void MainWindow::ShowData()
{
	// Clear treeview
	m_pcTree->Clear();
	
	// Load icons
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "plugins.png" );
	os::BitmapImage *pcPluginNodeIcon = new os::BitmapImage( pcStream );
	
	pcStream = cCol.GetResourceStream( "sounds.png" );
	os::BitmapImage *pcSoundsNodeIcon = new os::BitmapImage( pcStream );
	
	
	// Add plugins node
	os::TreeViewStringNode* pcPluginNode = new os::TreeViewStringNode();
	pcPluginNode->AppendString( os::String( "Plugins" ) );
	pcPluginNode->SetIndent( 1 );
	m_pcTree->InsertNode( pcPluginNode );
	pcPluginNode->SetExpanded( true );
	pcPluginNode->SetIcon( pcPluginNodeIcon );
	
	// Add inputs
	os::MediaInput* pcInput;
	uint32 nIndex = 0;
	m_nInputStart = 1;

	while ( ( pcInput = os::MediaManager::GetInstance()->GetInput( nIndex ) ) != NULL )
	{
		os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
		pcNode->AppendString( pcInput->GetIdentifier() );
		pcNode->SetIndent( 2 );
		m_pcTree->InsertNode( pcNode );
		delete( pcInput );
		nIndex++;
	}
	
	// Add codecs
	os::MediaCodec* pcCodec;
	m_nCodecStart = m_nInputStart + nIndex;
	nIndex = 0;

	while ( ( pcCodec = os::MediaManager::GetInstance()->GetCodec( nIndex ) ) != NULL )
	{
		os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
		pcNode->AppendString( pcCodec->GetIdentifier() );
		pcNode->SetIndent( 2 );
		m_pcTree->InsertNode( pcNode );
		delete( pcCodec );
		nIndex++;
	}
	
	// Add outputs
	os::MediaOutput * pcOutput;
	m_nOutputStart = m_nCodecStart + nIndex;
	nIndex = 0;

	while ( ( pcOutput = os::MediaManager::GetInstance()->GetOutput( nIndex ) ) != NULL )
	{
		os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
		pcNode->AppendString( pcOutput->GetIdentifier() );
		pcNode->SetIndent( 2 );
		
		if( pcOutput->GetIdentifier() == cCurrentVideo || pcOutput->GetIdentifier() == cCurrentAudio )
		{
			pcStream = cCol.GetResourceStream( "default.png" );
			os::BitmapImage *pcDefaultNodeIcon = new os::BitmapImage( pcStream );
			pcNode->SetIcon( pcDefaultNodeIcon );
		}
		m_pcTree->InsertNode( pcNode );
		
		delete( pcOutput );
		nIndex++;
	}
	
	// Add sounds node
	os::TreeViewStringNode* pcSoundsNode = new os::TreeViewStringNode();
	pcSoundsNode->AppendString( os::String( "Sounds" ) );
	pcSoundsNode->SetIndent( 1 );
	m_pcTree->InsertNode( pcSoundsNode );
	pcSoundsNode->SetExpanded( true );
	pcSoundsNode->SetIcon( pcSoundsNodeIcon );
	
	// Add startup node
	os::TreeViewStringNode* pcNode = new os::TreeViewStringNode();
	pcNode->AppendString( "Startup" );
	pcNode->SetIndent( 2 );
	m_pcTree->InsertNode( pcNode );
	
	
	if( m_nTreeSelect > -1 )
		m_pcTree->Select( m_nTreeSelect );
	Treeview();
}

void MainWindow::Apply()
{
	os::MediaManager::GetInstance()->SetDefaultAudioOutput( cCurrentAudio );
	os::MediaManager::GetInstance()->SetDefaultVideoOutput( cCurrentVideo );
	os::Message cMsg( os::MEDIA_SERVER_SET_STARTUP_SOUND );
	cMsg.AddString( "sound", cCurrentStartupSound );
	os::MediaManager::GetInstance()->GetServerLink(  ).SendMessage( &cMsg );
}

void MainWindow::Undo()
{
	// Copy original state back
	cCurrentVideo = cUndoVideo;
	cCurrentAudio = cUndoAudio;
	cCurrentStartupSound = cUndoStartupSound;

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

	// Update controls
	ShowData();
}

void MainWindow::Treeview()
{
	/* Delete old view */
	if( m_pcPrefs != NULL )
	{
		pcHLSettings->RemoveChild( m_pcPrefs );
		delete( m_pcPrefs );
		m_pcPrefs = NULL;
	}
	
	/* Look what has been selected */
	int nSelected = m_pcTree->GetFirstSelected();
	m_nTreeSelect = nSelected;
	
	if( nSelected >= m_nInputStart && nSelected < m_nCodecStart ) {
		pcLRoot->SetRoot( NULL );
		os::MediaInput* pcInput = os::MediaManager::GetInstance()->GetInput( nSelected - m_nInputStart );
		m_pcPrefs = new InputPrefs( pcInput );
		
		pcVLRoot->RemoveChild( pcHLSettings );
		pcVLRoot->RemoveChild( pcHLButtons );
		pcHLSettings->AddChild( m_pcPrefs );
		pcVLRoot->AddChild( pcHLSettings );
		pcVLRoot->AddChild( pcHLButtons );
		pcLRoot->SetRoot( pcVLRoot );
		pcLRoot->InvalidateLayout();
	} else if( nSelected >= m_nCodecStart && nSelected < m_nOutputStart ) {
		pcLRoot->SetRoot( NULL );		
		os::MediaCodec* pcCodec = os::MediaManager::GetInstance()->GetCodec( nSelected - m_nCodecStart );
		m_pcPrefs = new CodecPrefs( pcCodec );
		pcVLRoot->RemoveChild( pcHLSettings );
		pcVLRoot->RemoveChild( pcHLButtons );
		pcHLSettings->AddChild( m_pcPrefs );
		pcVLRoot->AddChild( pcHLSettings );
		pcVLRoot->AddChild( pcHLButtons );
		pcLRoot->SetRoot( pcVLRoot );
		pcLRoot->InvalidateLayout();
	} else if( nSelected >= m_nOutputStart && nSelected < ( (int)m_pcTree->GetRowCount() - 2 ) ) {
		pcLRoot->SetRoot( NULL );		
		os::MediaOutput* pcOutput = os::MediaManager::GetInstance()->GetOutput( nSelected - m_nOutputStart );
		m_pcPrefs = new OutputPrefs( pcOutput, pcOutput->GetIdentifier() == cCurrentVideo, pcOutput->GetIdentifier() == cCurrentAudio );
		pcVLRoot->RemoveChild( pcHLSettings );
		pcVLRoot->RemoveChild( pcHLButtons );
		pcHLSettings->AddChild( m_pcPrefs );
		pcVLRoot->AddChild( pcHLSettings );
		pcVLRoot->AddChild( pcHLButtons );
		pcLRoot->SetRoot( pcVLRoot );
		pcLRoot->InvalidateLayout();
	} else if( nSelected == (int)( m_pcTree->GetRowCount() - 1 ) ) {
		pcLRoot->SetRoot( NULL );		
		m_pcPrefs = new SoundPrefs( this, cCurrentStartupSound );
		pcVLRoot->RemoveChild( pcHLSettings );
		pcVLRoot->RemoveChild( pcHLButtons );
		pcHLSettings->AddChild( m_pcPrefs );
		pcVLRoot->AddChild( pcHLSettings );
		pcVLRoot->AddChild( pcHLButtons );
		pcLRoot->SetRoot( pcVLRoot );
		pcLRoot->InvalidateLayout();
	} else {
		pcLRoot->InvalidateLayout();
	}
	
}

void MainWindow::VideoOutputChange()
{
	int nSelected = m_pcTree->GetFirstSelected();
	os::MediaOutput* pcOutput = os::MediaManager::GetInstance()->GetOutput( nSelected - m_nOutputStart );
	cCurrentVideo = pcOutput->GetIdentifier();
	delete( pcOutput );
	ShowData();
}


void MainWindow::AudioOutputChange()
{
	int nSelected = m_pcTree->GetFirstSelected();
	os::MediaOutput* pcOutput = os::MediaManager::GetInstance()->GetOutput( nSelected - m_nOutputStart );
	cCurrentAudio = pcOutput->GetIdentifier();
	delete( pcOutput );
	ShowData();
}


void MainWindow::StartupSoundChange()
{
	cCurrentStartupSound = (static_cast<SoundPrefs*>(m_pcPrefs))->GetString();
	//std::cout<<cCurrentStartupSound.c_str()<<std::endl;
}

void MainWindow::HandleMessage( os::Message * pcMessage )
{
	// Get message code and act on it
	switch ( pcMessage->GetCode() )
	{

	case M_MW_VIDEOOUTPUT:
		VideoOutputChange();
		break;
	case M_MW_AUDIOOUTPUT:
		AudioOutputChange();
		break;
	case M_MW_STARTUPSOUND:
		StartupSoundChange();
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

	case M_MW_TREEVIEW:
		Treeview();
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














