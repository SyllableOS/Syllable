#ifndef __F_KEYBOARDPANEL_H_
#define __F_KEYBOARDPANEL_H_

#include <gui/layoutview.h>

#include <vector>
#include <map>

using namespace os;

namespace os {
  class Button;
  class ListView;
  class CheckBox;
  class DropdownMenu;
  class Spinner;
  class StringView;
}

class ColorEdit;

class KeyboardPanel : public os::LayoutView
{
public:
  
    KeyboardPanel( const os::Rect& cFrame );
    virtual void Paint( const os::Rect& cUpdateRect );
    virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void FrameSized( const os::Point& cDelta );
    virtual void AllAttached();
    virtual void HandleMessage( Message* pcMessage );
  
private:
//    void Layout();
    void UpdateResList();

    struct ColorSpace
    {
	ColorSpace( color_space eMode ) { m_eMode = eMode; }
	bool operator < ( const ColorSpace& cInst ) const { return( m_eMode < cInst.m_eMode ); }
	color_space		   m_eMode;
	std::vector<screen_mode> m_cResolutions;
    };

    std::map<color_space,ColorSpace> m_cColorSpaces;
  
    Button*	m_pcRefreshBut;
    Button*	m_pcOkBut;
    Button*	m_pcCancelBut;
    ListView*	m_pcKeymapList;
    Spinner*	m_pcKeyDelay;
    Spinner*	m_pcKeyRepeat;
    
};

#endif // __F_KEYBOARDPANEL_H_
