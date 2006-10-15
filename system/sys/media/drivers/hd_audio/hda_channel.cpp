/*  Syllable Audio Interface (HDA Codec)
 *  Copyright (C) 2006 Arno Klenke
 *
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
#include <sys/ioctl.h>

#include "hda.h"
#include "hda_channel.h"

using namespace os;

enum
{
	MESSAGE_CHANNEL_CHANGED,
	MESSAGE_LOCK_CHANGED
};



/* Config types */
static char* g_azConfigType[] =
{
	"Line out",
	"Speaker",
	"Headphone",
	"CD",
	"SPDIF out",
	"Other digital out",
	"Modem line",
	"Modem hand",
	"Line in",
	"Aux",
	"Mic in",
	"Telephony",
	"SPDIF in",
	"Other digital in"
};

/* Pin colors */
struct PinColor_s
{
	int nIndex;
	uint8 nPinR;
	uint8 nPinG;
	uint8 nPinB;
	uint8 nTextR;
	uint8 nTextG;
	uint8 nTextB;
};

struct PinColor_s g_asColors[] =
{
	{ 1, 0, 0, 0, 255, 255, 255 }, // Black
	{ 2, 200, 200, 200, 0, 0, 0 }, // Grey
	{ 3, 100, 100, 255, 255, 255, 255 }, // Blue
	{ 4, 100, 225, 100, 255, 255, 255 }, // Green
	{ 5, 255, 100, 100, 255, 255, 255 }, // Red
	{ 6, 255, 160, 100, 255, 255, 255 }, // Orange
	{ 7, 255, 255, 100, 255, 255, 255 }, // Yellow
	{ 8, 190, 0, 255, 255, 255, 255 }, // Purple
	{ 9, 255, 0, 255, 255, 255, 255 }, // Pink
	{ 0xfe, 0, 0, 0, 255, 255, 255 } // White
};

HDAChannel::HDAChannel( int nFd, const Rect &cFrame, const std::string &cTitle, HDAAudioDriver* pcDriver, HDAOutputPath_s sPath )
				:LayoutView( cFrame, cTitle, NULL, 0 )
{

	m_pcVNode = new VLayoutNode( "v_layout" );
	m_nFd = nFd;
	m_pcDriver = pcDriver;
	m_sPath = sPath;
	
	/* Read value */
	
	int nLeft = 0;
	int nRight = 0;
	
	
	os::HLayoutNode* pcHNode = new os::HLayoutNode( "h_node" );
	
	HDANode_s* psNode;
	psNode = m_pcDriver->GetNode( sPath.nPinNode );
	
	os::String zChannelName = "Unknown";
	if( psNode != NULL )
	{
		/* Set name */
		if( ( ( psNode->nDefCfg & AC_DEFCFG_DEVICE ) >> AC_DEFCFG_DEVICE_SHIFT ) < 0xff )
		{
			zChannelName = g_azConfigType[( psNode->nDefCfg & AC_DEFCFG_DEVICE ) >> AC_DEFCFG_DEVICE_SHIFT];
		}
		
		/* Search color */
		int nColor = ( psNode->nDefCfg & AC_DEFCFG_COLOR ) >> AC_DEFCFG_COLOR_SHIFT;
		uint i = 0;
		m_sPinColor = os::get_default_color( os::COL_NORMAL );
		m_sTextColor = os::Color32_s( 0, 0, 0 );
		for( i = 0; i < ( sizeof( g_asColors ) / sizeof( struct PinColor_s ) ); i++ )
		{
			if( g_asColors[i].nIndex == nColor )
			{
				m_sPinColor = os::Color32_s( g_asColors[i].nPinR, g_asColors[i].nPinG, g_asColors[i].nPinB );
				m_sTextColor = os::Color32_s( g_asColors[i].nTextR, g_asColors[i].nTextG, g_asColors[i].nTextB );
				break;
			}
		}
	}
	m_pcLabel = new StringView( os::Rect(), cTitle, zChannelName, os::ALIGN_CENTER );
	
	m_pcVNode->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	m_pcVNode->AddChild( m_pcLabel, 0.0f );
	
	if( sPath.nVolNode != -1 && ( psNode = m_pcDriver->GetNode( sPath.nVolNode ) ) != NULL )
	{
		if( psNode->nWidCaps & AC_WCAP_OUT_AMP )
		{
			int nSteps = ( psNode->nAmpOutCaps & AC_AMPCAP_NUM_STEPS) >> AC_AMPCAP_NUM_STEPS_SHIFT;
			HDACmd_s sCmd = { 0, psNode->nNode, 0, AC_VERB_GET_AMP_GAIN_MUTE, AC_AMP_GET_LEFT | AC_AMP_GET_OUTPUT, 0 };
			if( ioctl( nFd, HDA_CODEC_READ, &sCmd ) == 0 && nSteps != 0 )
				nLeft = sCmd.nResult * 100 / nSteps;
			sCmd.nPara = AC_AMP_GET_RIGHT | AC_AMP_GET_OUTPUT;
			if( ioctl( nFd, HDA_CODEC_READ, &sCmd ) == 0 && nSteps != 0 )
				nRight = sCmd.nResult * 100 / nSteps;
		}
		else if( psNode->nWidCaps & AC_WCAP_IN_AMP )
		{
			int nSteps = ( psNode->nAmpInCaps & AC_AMPCAP_NUM_STEPS) >> AC_AMPCAP_NUM_STEPS_SHIFT;
			HDACmd_s sCmd = { 0, psNode->nNode, 0, AC_VERB_GET_AMP_GAIN_MUTE, AC_AMP_GET_LEFT | AC_AMP_GET_INPUT, 0 };
			if( ioctl( nFd, HDA_CODEC_READ, &sCmd ) == 0 && nSteps != 0 )
				nLeft = sCmd.nResult * 100 / nSteps;
			sCmd.nPara = AC_AMP_GET_RIGHT | AC_AMP_GET_INPUT;
			if( ioctl( nFd, HDA_CODEC_READ, &sCmd ) == 0 && nSteps != 0 )
				nRight = sCmd.nResult * 100 / nSteps;
		}
	
	Message *pcMsgLeft = new Message( MESSAGE_CHANNEL_CHANGED );
	pcMsgLeft->AddBool( "left", true );

	
	m_pcLeft = new Slider( Rect(), cTitle + "left", pcMsgLeft, 
		Slider::TICKS_LEFT, 10, Slider::KNOB_SQUARE, VERTICAL);
	
	m_pcLeft->SetMinMax( 0, 100 );
	m_pcLeft->SetValue( nLeft );
	
	pcHNode->AddChild( m_pcLeft );

	Message *pcMsgRight = new Message( MESSAGE_CHANNEL_CHANGED );
	pcMsgRight->AddBool( "left", false );
	
	m_pcRight = new Slider( Rect(), cTitle + "right", pcMsgRight, 
		Slider::TICKS_RIGHT, 10, Slider::KNOB_SQUARE, VERTICAL);
	
	m_pcRight->SetMinMax( 0, 100 );
	m_pcRight->SetValue( nRight );
	
	pcHNode->AddChild( new os::HLayoutSpacer( "", 2.0f, 2.0f ) );
	
	pcHNode->AddChild( m_pcRight );
	
	
	m_pcVNode->AddChild( pcHNode );

	Message *pcMsgLock = new Message( MESSAGE_LOCK_CHANGED );
	
	os::HLayoutNode* pcLockHNode = new os::HLayoutNode( "lock_h_node", 0.0f );
	
	m_pcLock = new CheckBox( Rect(), "lock", "Lock", 
		pcMsgLock, 0, 0 );
	
	if( nLeft == nRight )
		m_pcLock->SetValue( 1 );
		
	pcLockHNode->AddChild( new os::HLayoutSpacer( "" ) );
	pcLockHNode->AddChild( m_pcLock, 0.0f );
	pcLockHNode->AddChild( new os::HLayoutSpacer( "" ) );	
	
	m_pcVNode->AddChild( pcLockHNode );
	
	m_pcVNode->AddChild( new os::VLayoutSpacer( "", 5.0f, 5.0f ) );
	}
	else
	{
		m_pcLeft = m_pcRight = NULL;
		m_pcLock = NULL;
		m_pcVNode->AddChild( new os::VLayoutSpacer( "" ) );
	}
	
	SetRoot( m_pcVNode );
	
}

HDAChannel::~HDAChannel()
{
}
void HDAChannel::AllAttached()
{
	if( m_pcLeft )
		m_pcLeft->SetTarget( this );
	if( m_pcRight )
		m_pcRight->SetTarget( this );
	if( m_pcLock )
		m_pcLock->SetTarget( this );
	m_pcLabel->SetBgColor( m_sPinColor );
	m_pcLabel->SetFgColor( m_sTextColor );
}

void HDAChannel::HandleMessage( os::Message* pcMsg )
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
			HDANode_s* psNode;
			if( m_sPath.nVolNode != -1 && ( psNode = m_pcDriver->GetNode( m_sPath.nVolNode ) ) != NULL )
			{
				if( psNode->nWidCaps & AC_WCAP_OUT_AMP )
				{
					int nSteps = ( psNode->nAmpOutCaps & AC_AMPCAP_NUM_STEPS) >> AC_AMPCAP_NUM_STEPS_SHIFT;
					uint32 nVal = AC_AMP_SET_LEFT | AC_AMP_SET_OUTPUT;
					if( nVal == 0 )
						nVal |= AC_AMP_MUTE;
					else
						nVal |= ( nLeft * nSteps / 100 );
					HDACmd_s sCmd = { 0, psNode->nNode, 0, AC_VERB_SET_AMP_GAIN_MUTE, nVal, 0 };
					ioctl( m_nFd, HDA_CODEC_SEND_CMD, &sCmd );
					nVal = AC_AMP_SET_RIGHT | AC_AMP_SET_OUTPUT;
					if( nVal == 0 )
						nVal |= AC_AMP_MUTE;
					else
						nVal |= ( nRight * nSteps / 100 );
					sCmd.nPara = nVal;
					ioctl( m_nFd, HDA_CODEC_SEND_CMD, &sCmd );

				}
				else if( psNode->nWidCaps & AC_WCAP_IN_AMP )
				{
					int nSteps = ( psNode->nAmpInCaps & AC_AMPCAP_NUM_STEPS) >> AC_AMPCAP_NUM_STEPS_SHIFT;
					uint32 nVal = AC_AMP_SET_LEFT | AC_AMP_SET_INPUT;
					if( nVal == 0 )
						nVal |= AC_AMP_MUTE;
					else
						nVal |= ( nLeft * nSteps / 100 );
					HDACmd_s sCmd = { 0, psNode->nNode, 0, AC_VERB_SET_AMP_GAIN_MUTE, nVal, 0 };
					ioctl( m_nFd, HDA_CODEC_SEND_CMD, &sCmd );
					nVal = AC_AMP_SET_RIGHT | AC_AMP_SET_INPUT;
					if( nVal == 0 )
						nVal |= AC_AMP_MUTE;
					else
						nVal |= ( nRight * nSteps / 100 );
					sCmd.nPara = nVal;
					ioctl( m_nFd, HDA_CODEC_SEND_CMD, &sCmd );
				}
			}
			#if 0
			if( m_bStereo )
				m_pcDriver->AC97WriteStereoMixer( m_nFd, m_nCodec, m_nReg, nLeft, nRight );
			else if( m_nReg == AC97_MIC_VOL )
				m_pcDriver->AC97WriteMicMixer( m_nFd, m_nCodec, nLeft );
			else
				m_pcDriver->AC97WriteMonoMixer( m_nFd, m_nCodec, m_nReg, nLeft );
			#endif
			//m_pcDriver->AC97SaveMixerSettings();
		}
		break;
		default:
			break;
	}
}

int HDAChannel::GetLeftValue()
{
	return( (int) m_pcLeft->GetValue().AsInt8() );
}

int HDAChannel::GetRightValue()
{
	if( m_pcRight )
		return( (int) m_pcRight->GetValue().AsInt8() );
	return( 0 );
}


void HDAChannel::SetLeftValue( int nValue )
{
	m_pcLeft->SetValue( nValue, true );
}

void HDAChannel::SetRightValue( int nValue )
{
	m_pcRight->SetValue( nValue, true );
}

int HDAChannel::GetLockValue()
{
	if( m_pcLock )
		return( (int) m_pcLock->GetValue().AsInt8() );
	return( 0 );
}


void HDAChannel::SetLockValue( int nValue )
{
	if( m_pcLock )
		m_pcLock->SetValue( nValue, true );
}










