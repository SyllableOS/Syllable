/*
 *  Mixer 0.1 
 *
 *  Copyright (C) 2001	Arno Klenke
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <atheos/soundcard.h>
#include <util/message.h>
#include <gui/view.h>
#include <gui/checkbox.h>
#include <gui/frameview.h>
#include <gui/window.h>
#include <gui/slider.h>
#include <gui/button.h>

#include "mixerchannel.h"

using namespace os;

MixerChannel::MixerChannel( const Rect &cFrame, const std::string &cTitle, Window *pcTarget, int nChannel, int nMixer )
				:View( cFrame, cTitle )
{
	
	Message *pcMsgLeft = new Message( MESSAGE_CHANNEL_CHANGED );
	pcMsgLeft->AddInt8( "channel", nChannel );
	pcMsgLeft->AddBool( "left", true );
	pcMsgLeft->AddInt8( "mixer", nMixer );

	
	m_zLabel = cTitle;
	m_pcString = new StringView( Rect( 5, 0, 40, 32 ), "label", m_zLabel.c_str() );
	AddChild( m_pcString );

	
	m_pcLeft = new Slider( Rect( 40, 0, 180, 20 ), "left", pcMsgLeft, 
		Slider::TICKS_ABOVE, 10, Slider::KNOB_SQUARE, HORIZONTAL );
	
	m_pcLeft->SetMinMax( 1, 100 );
	m_pcLeft->SetValue( 1 );
	m_pcLeft->SetTarget( pcTarget );
	m_pcLeft->SetSliderColors( get_default_color( COL_SEL_MENU_BACKGROUND ),
									get_default_color( COL_NORMAL ) );
	m_pcLeft->FrameSized( Point( 25, 160 ) );
	
	
	Message *pcMsgRight = new Message( MESSAGE_CHANNEL_CHANGED );
	pcMsgRight->AddInt8( "channel", nChannel );
	pcMsgRight->AddBool( "left", false );
	pcMsgRight->AddInt8( "mixer", nMixer );

	m_pcRight = new Slider( Rect( 40, 20, 180, 40 ), "right", pcMsgRight, 
		Slider::TICKS_BELOW, 10, Slider::KNOB_SQUARE, HORIZONTAL );
	
	m_pcRight->SetMinMax( 1, 100 );
	m_pcRight->SetValue( 1 );
	m_pcRight->SetTarget( pcTarget );
	m_pcRight->SetSliderColors( get_default_color( COL_SEL_MENU_BACKGROUND ),
									get_default_color( COL_NORMAL ) );
	m_pcRight->FrameSized( Point( 25, 160 ) );
	
	
	Message *pcMsgLock = new Message( MESSAGE_LOCK_CHANGED );
	pcMsgLock->AddInt8( "channel", nChannel );
	pcMsgLock->AddInt8( "mixer", nMixer );

	m_pcLock = new CheckBox( Rect( 185, 10, 240, 25 ), "lock", "Lock", 
		pcMsgLock, 0, 0 ); 
	
	m_nChannel = nChannel;
	m_nMixer = nMixer;

	AddChild( m_pcLeft );
	AddChild( m_pcRight );
	AddChild( m_pcLock );
}

MixerChannel::~MixerChannel()
{
	RemoveChild( m_pcLeft );
	RemoveChild( m_pcRight );
}


int MixerChannel::GetLeftValue()
{
	return( (int) m_pcLeft->GetValue().AsInt8() );
}

int MixerChannel::GetRightValue()
{
	return( (int) m_pcRight->GetValue().AsInt8() );
}


void MixerChannel::SetLeftValue( int nValue )
{
	m_pcLeft->SetValue( nValue, true );
	m_pcLeft->FrameSized( Point( 25, 160 ) );
}

void MixerChannel::SetRightValue( int nValue )
{
	m_pcRight->SetValue( nValue, true );
	m_pcRight->FrameSized( Point( 25, 160 ) );
}

int MixerChannel::GetLockValue()
{
	return( (int) m_pcLock->GetValue().AsInt8() );
}


void MixerChannel::SetLockValue( int nValue )
{
	m_pcLock->SetValue( nValue, true );
}





