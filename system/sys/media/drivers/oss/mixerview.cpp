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

#include <util/string.h>
#include <util/message.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <atheos/soundcard.h>
#include <gui/view.h>
#include <gui/checkbox.h>
#include <gui/frameview.h>
#include <gui/slider.h>
#include <gui/button.h>

#include "mixerchannel.h"
#include "mixerview.h"
#include "mixermsg.h"

using namespace os;


MixerView::MixerView( String zPath, Rect cFrame )
				:View( cFrame, "view", 0, WID_WILL_DRAW )
{
	float nXPos = 2;
	unsigned int nSetting;
	int nVolume;
	int nLeft, nRight;
	static const char *zSources[] = SOUND_DEVICE_LABELS;
	mixer_info sInfo;
	
	m_nMixer = open( zPath.c_str(), O_RDWR );
	
	ioctl( m_nMixer, SOUND_MIXER_INFO, &sInfo );
	
	ioctl( m_nMixer, MIXER_READ(SOUND_MIXER_DEVMASK), &nSetting);
	
	for ( int i = 0; i < SOUND_MIXER_NRDEVICES; i++ ) {
		m_pcChannel[i] = NULL;
		if ( nSetting & (1 << i) ) {
			if (ioctl( m_nMixer, MIXER_READ(i), &nVolume ) > -1 ) {
			
				m_pcChannel[i] = new Channel( Rect(nXPos, 0, nXPos + 50, 200), 
				zSources[i], this, i);
			
				nLeft  = nVolume & 0x00FF;
				nRight = (nVolume & 0xFF00) >> 8;
				
				m_pcChannel[i]->SetLeftValue( nLeft );
				m_pcChannel[i]->SetRightValue( nRight );
			
				if( nLeft == nRight )
					m_pcChannel[i]->SetLockValue( 1 );
				else
					m_pcChannel[i]->SetLockValue( 0 );
				
				AddChild(m_pcChannel[i]);
				nXPos += 52;
			}
		}
	}
	m_nWidth = (uint32)nXPos;
	SetFrame( Rect( 0, 0, nXPos, 201 ) );
}

MixerView::~MixerView( void )
{
	close( m_nMixer );
	for ( int i = 0; i < SOUND_MIXER_NRDEVICES; i++ ) {
		if( m_pcChannel[i] != NULL ) {
			RemoveChild( m_pcChannel[i] );
			delete( m_pcChannel[i] );
		}
	}
}

void MixerView::AttachedToWindow()
{
	for ( int i = 0; i < SOUND_MIXER_NRDEVICES; i++ ) {
		if( m_pcChannel[i] != NULL )
			m_pcChannel[i]->SetTargets( this );
	}
}

bool MixerView::OkToQuit( void )
{
	return( true );
}

void MixerView::HandleMessage( Message* pcMessage)
{
	int8 nChannel;
	int nLeft, nRight;
	switch( pcMessage->GetCode() )
	{
		case MESSAGE_LOCK_CHANGED:
			if( !pcMessage->FindInt8( "channel", &nChannel ) ) {
				if( m_pcChannel[nChannel]->GetLockValue() == 1 )
				{
					nLeft = m_pcChannel[nChannel]->GetLeftValue();
					nRight = m_pcChannel[nChannel]->GetRightValue();
					
					m_pcChannel[nChannel]->SetLeftValue( ( nLeft + nRight ) / 2 );
					m_pcChannel[nChannel]->SetRightValue( ( nLeft + nRight ) / 2 );
				}
			}
			break;
		
		case MESSAGE_CHANNEL_CHANGED:
			bool bLeftSlider;
			int nSetting, nVolume;
			if( !pcMessage->FindInt8( "channel", &nChannel ) ) {
			
				nLeft = m_pcChannel[nChannel]->GetLeftValue();
				nRight = m_pcChannel[nChannel]->GetRightValue();
				
				if( m_pcChannel[nChannel]->GetLockValue() == 1 ) {
					if( !pcMessage->FindBool( "left", &bLeftSlider ) ) {
						if( bLeftSlider == true ) 
							nRight = nLeft;
						else 
							nLeft = nRight;
					}
				}	
				
				nSetting = ( nLeft & 0xFF ) + ( ( nRight & 0xFF ) << 8 );
				ioctl( m_nMixer, MIXER_WRITE( nChannel ), &nSetting );
				
				ioctl( m_nMixer, MIXER_READ( nChannel ), &nVolume );		
				nLeft  = nVolume & 0x00FF;
				nRight = (nVolume & 0xFF00) >> 8;
				
				m_pcChannel[nChannel]->SetLeftValue( nLeft );
				m_pcChannel[nChannel]->SetRightValue( nRight );
			}
			break;
			
		default:
			View::HandleMessage( pcMessage );
	}

}





