#ifndef _MIXERCHANNEL_H_
#define _MIXERCHANNEL_H_

#include <gui/frameview.h>
#include <gui/slider.h>
#include <gui/checkbox.h>

namespace os
{

class Channel : public FrameView
{
public:
	Channel( const Rect &cFrame, const std::string &cTitle, View* pcTarget, int nChannel );
	~Channel();
	int GetLeftValue();
	int GetRightValue();
	void SetLeftValue( int nValue );
	void SetRightValue( int nValue );
	int GetLockValue();
	void SetLockValue( int nValue );
	void SetTargets( View* pcView ) { m_pcLeft->SetTarget( pcView ); m_pcRight->SetTarget( pcView ); }
private:
	Slider* m_pcLeft;
	Slider* m_pcRight;
	CheckBox *m_pcLock;
	int m_nChannel;
};

}

#endif





