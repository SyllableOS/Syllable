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

#include <iostream.h>

#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <atheos/soundcard.h>
#include <util/application.h>
#include <util/message.h>
#include <gui/view.h>
#include <gui/checkbox.h>
#include <gui/frameview.h>
#include <gui/window.h>
#include <gui/slider.h>
#include <gui/button.h>

#include "channel.h"
#include "view.h"
#include "window.h"
#include "messages.h"

MixerWindow::MixerWindow( Rect cFrame )
		:Window( cFrame, "mixer", "Mixer", WND_NOT_RESIZABLE )
{
	float nXPos = 2;
	unsigned int nSetting;
	int nVolume;
	int nLeft, nRight;
	static const char *zSources[] = SOUND_DEVICE_LABELS;
	mixer_info sInfo;
	
	m_pcView = new MixerView( GetBounds() );
	
	m_nMixer = open( "/dev/sound/mixer", O_RDWR );
	
	ioctl( m_nMixer, SOUND_MIXER_INFO, &sInfo );
	
	SetTitle( sInfo.name );
	
	ioctl( m_nMixer, MIXER_READ(SOUND_MIXER_DEVMASK), &nSetting);
	
	for ( int i = 0; i < SOUND_MIXER_NRDEVICES; i++ ) {
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
				
				m_pcView->AddChild(m_pcChannel[i]);
				nXPos += 52;
			}
		}
	}
	
	SetFrame( Rect( 100, 100, nXPos + 100, 301 ) );
	m_pcView->SetFrame( Rect( 0, 0, nXPos + 52, 201 ) );	
	AddChild( m_pcView );
}

MixerWindow::~MixerWindow( void )
{
	close( m_nMixer );
}

bool MixerWindow::OkToQuit( void )
{
	Application::GetInstance()->PostMessage( M_QUIT );
	return( true );
}

void MixerWindow::HandleMessage( Message* pcMessage)
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
			Window::HandleMessage( pcMessage );
	}

}

