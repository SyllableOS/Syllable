#ifndef _MIXERCHANNEL_H_
#define _MIXERCHANNEL_H_

using namespace os;

class Channel : public FrameView
{
public:
	Channel( const Rect &cFrame, const std::string &cTitle, Window* pcTarget, int nChannel );
	~Channel();
	int GetLeftValue();
	int GetRightValue();
	void SetLeftValue( int nValue );
	void SetRightValue( int nValue );
	int GetLockValue();
	void SetLockValue( int nValue );

private:
	Slider* m_pcLeft;
	Slider* m_pcRight;
	CheckBox *m_pcLock;
	int m_nChannel;
};

#endif
