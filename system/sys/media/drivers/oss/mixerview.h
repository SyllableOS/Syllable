#ifndef _MIXERVIEW_H_
#define _MIXERVIEW_H_

#include "mixerchannel.h"

namespace os
{

class MixerView : public View
{
	public:
		MixerView( String zPath, Rect cFrame );
		~MixerView();	
		void AttachedToWindow();
		bool OkToQuit( void );
		void HandleMessage( Message* pcMessage );
		virtual Point GetPreferredSize( bool bLargest ) const { if( bLargest ) return( Point( m_nWidth, 200 ) ); return( Point( 0, 200 ) ); }
	private:
		MixerView* m_pcView;
		Channel* m_pcChannel[SOUND_MIXER_NRDEVICES];
		int m_nMixer;
		uint32 m_nWidth;
};

}

#endif




