#ifndef __F_SCREENPANEL_H_
#define __F_SCREENPANEL_H_

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

class ScreenPanel : public os::LayoutView
{
public:
  
    ScreenPanel( const os::Rect& cFrame );
    virtual void Paint( const os::Rect& cUpdateRect );
    virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void FrameSized( const os::Point& cDelta );
    virtual void AllAttached();
    virtual void HandleMessage( Message* pcMessage );
  
private:
    void UpdateResList();

    struct ColorSpace
    {
	ColorSpace( color_space eMode ) { m_eMode = eMode; }
	bool operator < ( const ColorSpace& cInst ) const { return( m_eMode < cInst.m_eMode ); }
	color_space		   m_eMode;
	std::vector<screen_mode> m_cResolutions;
    };

    screen_mode m_sOriginalMode;
    screen_mode m_sCurrentMode;
    
    std::map<color_space,ColorSpace> m_cColorSpaces;
  
    Button*	m_pcOkBut;
    Button*	m_pcCancelBut;
    CheckBox*	m_pcAllCheckBox;
    DropdownMenu*	m_pcColorSpaceList;
    Spinner*	m_pcRefreshRate;

    StringView*	m_pcColorSpaceStr;
    StringView*	m_pcRefreshRateStr;
  
    ListView*	m_pcModeList;
};

#endif // __F_SCREENPANEL_H_
