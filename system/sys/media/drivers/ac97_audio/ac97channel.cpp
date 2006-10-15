/*  Syllable Audio Interface (AC97 Codec)
 *  Copyright (C) 2006 Arno Klenke
 *
 *  Copyright 2000 Silicon Integrated System Corporation
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

#include <util/message.h>
#include <gui/view.h>
#include <gui/checkbox.h>
#include <gui/frameview.h>
#include <gui/window.h>
#include <gui/slider.h>
#include <gui/button.h>

#include "ac97.h"
#include "ac97channel.h"

using namespace os;

enum
{
	MESSAGE_CHANNEL_CHANGED,
	MESSAGE_LOCK_CHANGED,
	MESSAGE_REC_SELECT
};

struct AC97MixerReg
{
	uint8 nReg;
	const char* pzName;
};

static AC97MixerReg g_asRegs[] =
{
	{ AC97_MASTER_VOL_STEREO, "Master" },
	{ AC97_SURROUND_MASTER, "Surround" },
	{ AC97_CENTER_LFE_MASTER, "Center/LFE" },
	{ AC97_HEADPHONE_VOL, "Headphone" },
	{ AC97_PCMOUT_VOL, "PCM" },
	{ AC97_LINEIN_VOL, "Line In" },
	{ AC97_CD_VOL, "CD" },
	{ AC97_AUX_VOL, "Aux" },
	{ AC97_MIC_VOL, "Mic" },
	{ AC97_RECORD_GAIN, "Record" },
	{ AC97_RECORD_GAIN_MIC, "Record Mic" }
};

struct AC97RecSelect
{
	uint16 nVal;
	const char* pzName;
};

static AC97RecSelect g_asRecSelect[] =
{
	{ AC97_RECMUX_MIC, "Mic" },
	{ AC97_RECMUX_CD, "CD" },
	{ AC97_RECMUX_VIDEO, "Video" },
	{ AC97_RECMUX_AUX, "Aux" },
	{ AC97_RECMUX_LINE, "Line" },
	{ AC97_RECMUX_STEREO_MIX, "Stereo Mix" },
	{ AC97_RECMUX_MONO_MIX, "Mono Mix" },
	{ AC97_RECMUX_PHONE, "Phone" }
};

AC97Channel::AC97Channel( int nFd, const Rect &cFrame, const std::string &cTitle, AC97AudioDriver* pcDriver, int nCodec, 
	uint8 nReg, bool bStereo )
				:LayoutView( cFrame, cTitle, NULL, 0 )
{
	m_pcVNode = new VLayoutNode( "v_layout" );
	m_nFd = nFd;
	m_pcDriver = pcDriver;
	m_nCodec = nCodec;
	m_nReg = nReg;
	m_bStereo = bStereo;
	
	/* Read value */
	int nLeft = 0;
	int nRight = 0;
	
	if( bStereo )
		m_pcDriver->AC97ReadStereoMixer( nFd, nCodec, nReg, &nLeft, &nRight );
	else if( nReg == AC97_MIC_VOL )
		m_pcDriver->AC97ReadMicMixer( nFd, nCodec, &nLeft );
	else
		m_pcDriver->AC97ReadMonoMixer( nFd, nCodec, nReg, &nLeft );
	
	os::HLayoutNode* pcHNode = new os::HLayoutNode( "h_node" );
	
	os::String zMixerName = "Unknown";
	for( uint i = 0; i < sizeof( g_asRegs ) / sizeof( AC97MixerReg ); i++ )
	{
		if( g_asRegs[i].nReg == nReg )
		{
			zMixerName = g_asRegs[i].pzName;
			break;
		}
	}
	

	m_pcLabel = new StringView( os::Rect(), cTitle, zMixerName, os::ALIGN_CENTER );
	m_pcVNode->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVNode->AddChild( m_pcLabel, 0.0f );

	Message *pcMsgLeft = new Message( MESSAGE_CHANNEL_CHANGED );
	pcMsgLeft->AddBool( "left", true );

	
	m_pcLeft = new Slider( Rect(), cTitle + "left", pcMsgLeft, 
		Slider::TICKS_LEFT, 10, Slider::KNOB_SQUARE, VERTICAL);
	
	m_pcLeft->SetMinMax( 0, 100 );
	m_pcLeft->SetValue( nLeft );
	
	pcHNode->AddChild( m_pcLeft );
	
	if( bStereo )
	{
		Message *pcMsgRight = new Message( MESSAGE_CHANNEL_CHANGED );
		pcMsgRight->AddBool( "left", false );
	
		m_pcRight = new Slider( Rect(), "right", pcMsgRight, 
			Slider::TICKS_RIGHT, 10, Slider::KNOB_SQUARE, VERTICAL);
	
		m_pcRight->SetMinMax( 0, 100 );
		m_pcRight->SetValue( nRight );
	
		pcHNode->AddChild( new os::HLayoutSpacer( "", 2.0f, 2.0f ) );
		pcHNode->AddChild( m_pcRight );
	} else 
		m_pcRight = NULL;
	m_pcVNode->AddChild( pcHNode );
	
	
	Message *pcMsgLock = new Message( MESSAGE_LOCK_CHANGED );
	
	os::HLayoutNode* pcLockHNode = new os::HLayoutNode( "lock_h_node", 0.0f );
	
	m_pcLock = new CheckBox( Rect(), "lock", "Lock", 
		pcMsgLock, 0, 0 );
	
	if( bStereo ) {
		if( nLeft == nRight )
			m_pcLock->SetValue( 1 );
	}
	else
		m_pcLock->SetEnable( false );
		
	pcLockHNode->AddChild( new os::HLayoutSpacer( "" ) );
	pcLockHNode->AddChild( m_pcLock, 0.0f );
	pcLockHNode->AddChild( new os::HLayoutSpacer( "" ) );	
	m_pcVNode->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVNode->AddChild( pcLockHNode );
	m_pcVNode->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	
	
	if( nReg == AC97_RECORD_GAIN )
	{
		/* Read record selection */
		
		uint16 nRecSelect = m_pcDriver->AC97Read( nFd, nCodec, AC97_RECORD_SELECT );
		printf( "%x\n", nRecSelect );
		/* Create dropdown menu */
		m_pcRecSelect = new os::DropdownMenu( os::Rect(), "rec_select" );
		m_pcRecSelect->SetReadOnly( true );
		m_pcRecSelect->SetMaxPreferredSize( 12 );
		for( uint i = 0; i < sizeof( g_asRecSelect ) / sizeof( AC97RecSelect ); i++ )
		{
			m_pcRecSelect->AppendItem( g_asRecSelect[i].pzName );
			if( g_asRecSelect[i].nVal == nRecSelect )
				m_pcRecSelect->SetSelection( i );
		}
		m_pcRecSelect->SetSelectionMessage( new os::Message( MESSAGE_REC_SELECT ) );
		m_pcVNode->AddChild( m_pcRecSelect );
		m_pcVNode->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	}
	
	SetRoot( m_pcVNode );
}

AC97Channel::~AC97Channel()
{
}
void AC97Channel::AttachedToWindow()
{
	m_pcLeft->SetTarget( this );
	if( m_pcRight )
		m_pcRight->SetTarget( this );
	if( m_pcLock )
		m_pcLock->SetTarget( this );
	if( m_nReg == AC97_RECORD_GAIN )
		m_pcRecSelect->SetTarget( this );
}

void AC97Channel::HandleMessage( os::Message* pcMsg )
{
	int nLeft, nRight;
	switch( pcMsg->GetCode() )
	{
		case MESSAGE_LOCK_CHANGED:
		{
			int nLeft, nRight;
			if( GetLockValue() == 1 )
			{
				nLeft = GetLeftValue();
				nRight = GetRightValue();
					
				SetLeftValue( ( GetLeftValue() + GetRightValue() ) / 2 );
				SetRightValue( ( GetLeftValue() + GetRightValue() ) / 2 );
			}
		}
		break;
		case MESSAGE_CHANNEL_CHANGED:
		{
			/* Set value */
			bool bLeftSlider;
			nLeft = GetLeftValue();
			nRight = GetRightValue();
			if( GetLockValue() == 1 && pcMsg->FindBool( "left", &bLeftSlider ) == 0 )
			{
				if( bLeftSlider )
				{
					nRight = nLeft;
					m_pcRight->SetValue( m_pcLeft->GetValue(), false );
				}
				else
				{
					nLeft = nRight;
					m_pcLeft->SetValue( m_pcRight->GetValue(), false );
				}
			}
			if( m_bStereo )
				m_pcDriver->AC97WriteStereoMixer( m_nFd, m_nCodec, m_nReg, nLeft, nRight );
			else if( m_nReg == AC97_MIC_VOL )
				m_pcDriver->AC97WriteMicMixer( m_nFd, m_nCodec, nLeft );
			else
				m_pcDriver->AC97WriteMonoMixer( m_nFd, m_nCodec, m_nReg, nLeft );
			//m_pcDriver->AC97SaveMixerSettings();
		}
		break;
		case MESSAGE_REC_SELECT:
		{
			printf("Here!\n");
			/* Record selection */
			int nSelect = m_pcRecSelect->GetSelection();
			if( nSelect < 0 )
				break;
			printf( "Write %x\n", g_asRecSelect[nSelect].nVal );
			m_pcDriver->AC97Write( m_nFd, m_nCodec, AC97_RECORD_SELECT, g_asRecSelect[nSelect].nVal );
		}
		break;
		default:
			break;
	}
}

int AC97Channel::GetLeftValue()
{
	return( (int) m_pcLeft->GetValue().AsInt8() );
}

int AC97Channel::GetRightValue()
{
	if( m_pcRight )
		return( (int) m_pcRight->GetValue().AsInt8() );
	return( 0 );
}


void AC97Channel::SetLeftValue( int nValue )
{
	m_pcLeft->SetValue( nValue, true );
}

void AC97Channel::SetRightValue( int nValue )
{
	m_pcRight->SetValue( nValue, true );
}

int AC97Channel::GetLockValue()
{
	if( m_pcLock )
		return( (int) m_pcLock->GetValue().AsInt8() );
	return( 0 );
}


void AC97Channel::SetLockValue( int nValue )
{
	if( m_pcLock )
		m_pcLock->SetValue( nValue, true );
}


























































