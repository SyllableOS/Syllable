/*  Media Server output plugin
 *  Copyright (C) 2003 Arno Klenke
 *	Based on the preferences code by Daryl Dudey
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


#include <gui/window.h>
#include <media/output.h>
#include <media/server.h>
#include <media/manager.h>
#include <util/messenger.h>
#include <util/message.h>
#include <atheos/msgport.h>
#include <atheos/areas.h>
#include <iostream>

#include "prefs.h"

enum {
	M_MW_SOUNDCARD,
	M_MW_APPLY
};

PrefsView::PrefsView( os::Rect cFrame )
				:os::View( cFrame, "prefs_view", 0, os::WID_WILL_DRAW )
{
	os::Rect cBounds = GetBounds();
	
	m_nWidth = 400;
	
	// Create the layouts/views
	m_pcLRoot = new os::LayoutView( cBounds, "L", NULL, os::CF_FOLLOW_ALL );
	m_pcVLRoot = new os::VLayoutNode( "VL" );
	m_pcVLRoot->SetBorders( os::Rect( 10, 5, 10, 5 ) );
	
	// Create Soundcard dropdown menu
	m_pcControls = new os::HLayoutNode( "HLControls" );
	m_pcSoundcard = new os::DropdownMenu( os::Rect(), "DDMDefaultDSP" );
//	m_pcSoundcard->SetMinPreferredSize( 5 );
	m_pcSoundcard->SetMaxPreferredSize( 22 );
	m_pcSoundcard->SetReadOnly( true );
	m_pcSoundcard->SetSelectionMessage( new os::Message( M_MW_SOUNDCARD ) );
	m_pcSoundcard->SetTarget( this );
	m_pcControls->AddChild( new os::StringView( os::Rect(), "SVSoundcard", "Default Soundcard" ) );
	m_pcControls->AddChild( new os::HLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcControls->AddChild( m_pcSoundcard );
	m_pcControls->AddChild( new os::HLayoutSpacer( "" ) );
	
	m_pcVLRoot->AddChild( m_pcControls );
	m_pcVLRoot->AddChild( new os::VLayoutSpacer( "" ) );

	// Create Apply button
	m_pcButtons = new os::HLayoutNode( "HLButtons" );
	m_pcButtons->AddChild( m_pcApply = new os::Button( os::Rect(), "BApply", "Apply", 
							new os::Message( M_MW_APPLY ) ) )->LimitMaxSize( m_pcApply->GetPreferredSize( false ) );
	m_pcButtons->AddChild( new os::HLayoutSpacer( "" ) );
	m_pcVLRoot->AddChild( m_pcButtons );
	
	
	m_pcLRoot->SetRoot( m_pcVLRoot );
	AddChild( m_pcLRoot );
	m_pcSoundcard->MakeFocus();
	
	
	// Default Soundcard
	os::Message cReply;
	os::MediaManager::GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_GET_DSP_COUNT, &cReply );
	int32 nCount, hDefault;

	if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindInt32( "dsp_count", &nCount ) == 0 )
	{
		os::MediaManager::GetInstance()->GetServerLink().SendMessage( os::MEDIA_SERVER_GET_DEFAULT_DSP, &cReply );
		if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindInt32( "handle", &hDefault ) == 0 )
		{
			m_hCurrentSoundcard = hDefault;
		}
		else
			m_hCurrentSoundcard = 0;
	}
	else	// No DSP's have been found by the server
	{
		nCount = 0;
		m_hCurrentSoundcard = 0;
		m_pcSoundcard->SetEnable( false );
	}
	
	// Fill in Soundcard entries
	if( nCount > 0 )
	{
		std::string cName;

		for( int32 j=0; j < nCount; j++ )
		{
			os::Message cReply;
			os::Message cMsg( os::MEDIA_SERVER_GET_DSP_INFO );
			cMsg.AddInt32( "handle", j );
			os::MediaManager::GetInstance()->GetServerLink().SendMessage( &cMsg, &cReply );

			if ( cReply.GetCode() == os::MEDIA_SERVER_OK && cReply.FindString( "name", &cName ) == 0 )
				m_pcSoundcard->AppendItem( cName.c_str() );
		}
		m_pcSoundcard->SetSelection( m_hCurrentSoundcard, false );
	}
}

PrefsView::~PrefsView()
{
}

void PrefsView::AttachedToWindow()
{
	(GetWindow())->SetDefaultButton( m_pcApply );
	m_pcSoundcard->SetTarget( this );
	m_pcApply->SetTarget( this );
}


bool PrefsView::OkToQuit( void )
{
	return( true );
}


void PrefsView::HandleMessage( os::Message* pcMessage)
{
	switch( pcMessage->GetCode() )
	{
		case M_MW_SOUNDCARD:
		{
			m_hCurrentSoundcard = m_pcSoundcard->GetSelection();
			break;
		}
		case M_MW_APPLY:
		{
			os::Message cDspMsg( os::MEDIA_SERVER_SET_DEFAULT_DSP );
			cDspMsg.AddInt32( "handle", m_hCurrentSoundcard );
			os::MediaManager::GetInstance()->GetServerLink(  ).SendMessage( &cDspMsg );
			break;
		}
		default:
			os::View::HandleMessage( pcMessage );
	}
}














