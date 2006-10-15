#ifndef _MIXERCHANNEL_H_
#define _MIXERCHANNEL_H_

#include <gui/frameview.h>
#include <gui/slider.h>
#include <gui/checkbox.h>
#include <gui/layoutview.h>
#include <gui/stringview.h>
#include <gui/dropdownmenu.h>

namespace os
{

class HDAChannel : public LayoutView
{
public:
	HDAChannel( int nFd, const Rect &cFrame, const std::string &cTitle, HDAAudioDriver* pcDriver,
	HDAOutputPath_s sPath );
	~HDAChannel();
	int GetLeftValue();
	int GetRightValue();
	void SetLeftValue( int nValue );
	void SetRightValue( int nValue );
	int GetLockValue();
	void SetLockValue( int nValue );
private:
	void AllAttached();
	void HandleMessage( os::Message* pcMsg );
	VLayoutNode *m_pcVNode;
	os::Color32_s m_sPinColor;
	os::Color32_s m_sTextColor;
	StringView* m_pcLabel;
	Slider* m_pcLeft;
	Slider* m_pcRight;
	CheckBox *m_pcLock;
	HDAAudioDriver* m_pcDriver;
	int m_nFd;
	HDAOutputPath_s m_sPath;
};

}

#endif






















