#ifndef _MIXERCHANNEL_H_
#define _MIXERCHANNEL_H_

#include <gui/stringview.h>
#include <gui/slider.h>
#include <gui/checkbox.h>

#define MESSAGE_CHANNEL_CHANGED 0
#define MESSAGE_LOCK_CHANGED 1

class MixerChannel : public os::View
{
public:
	MixerChannel( const os::Rect &cFrame, const std::string &cTitle, os::Window* pcTarget, int nChannel, int nMixer );
	~MixerChannel();
	int GetLeftValue();
	int GetRightValue();
	void SetLeftValue( int nValue );
	void SetRightValue( int nValue );
	int GetLockValue();
	void SetLockValue( int nValue );

private:
	os::String m_zLabel;
	os::StringView* m_pcString;
	os::Slider* m_pcLeft;
	os::Slider* m_pcRight;
	os::CheckBox *m_pcLock;
	int m_nChannel;
	int m_nMixer;
};

#endif








