//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free
//  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//  MA 02111-1307, USA
//

#ifndef _MIXERWINDOW_H_
#define _MIXERWINDOW_H_

using namespace os;

class MixerWindow : public Window
{
	public:
		MixerWindow( Rect cFrame );
		~MixerWindow( void );
		bool OkToQuit( void );
		void HandleMessage( Message* pcMessage );
	private:
		MixerView* m_pcView;
		Channel* m_pcChannel[SOUND_MIXER_NRDEVICES];
		int m_nMixer;
};

#endif