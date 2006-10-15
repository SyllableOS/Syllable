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

class AC97Channel : public LayoutView
{
public:
	AC97Channel( int nFd, const Rect &cFrame, const std::string &cTitle, AC97AudioDriver* pcDriver,
	int nCodec, uint8 nReg, bool bStereo );
	~AC97Channel();
	int GetLeftValue();
	int GetRightValue();
	void SetLeftValue( int nValue );
	void SetRightValue( int nValue );
	int GetLockValue();
	void SetLockValue( int nValue );
private:
	void AttachedToWindow();
	void HandleMessage( os::Message* pcMsg );
	VLayoutNode *m_pcVNode;
	StringView* m_pcLabel;
	Slider* m_pcLeft;
	Slider* m_pcRight;
	CheckBox *m_pcLock;
	DropdownMenu* m_pcRecSelect;
	AC97AudioDriver* m_pcDriver;
	int m_nFd;
	int m_nCodec;
	uint8 m_nReg;
	bool m_bStereo;
};

}

#endif



















