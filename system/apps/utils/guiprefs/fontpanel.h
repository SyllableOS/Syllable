#ifndef __F_FONTPANEL_H__
#define __F_FONTPANEL_H__

#include <gui/layoutview.h>
#include <gui/font.h>

#include <map>

using namespace os;

namespace os {
  class Button;
  class ListView;
  class Spinner;
  class TextView;
  class DropdownMenu;
}
enum
{
  MID_ABOUT,
  MID_HELP,

  MID_LOAD,
  MID_INSERT,
  MID_SAVE,
  MID_SAVE_AS,
  MID_DUP_SEL,
  MID_DELETE_SEL,
  MID_RESTORE,

  MID_SNAPSHOT,
  MID_RESET,
  MID_QUIT,

  GID_COUNT
};

class FontPanel : public os::LayoutView
{
public:
  
    FontPanel( const os::Rect& cFrame );
    virtual void Paint( const os::Rect& cUpdateRect );
    virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void FrameSized( const os::Point& cDelta );
    virtual void AllAttached();
    virtual void HandleMessage( Message* pcMessage );
  
private:
    void UpdateSelectedConfig();

    Rect	  m_cTestArea;

    struct ConfigNode {
	std::string	    m_cName;
	os::font_properties m_sConfig;
	os::font_properties m_sOrigConfig;
	bool		    m_bModified;
    };
    Font::size_list_t	    m_cBitmapSizes;
    std::vector<ConfigNode> m_cConfigList;
    int			    m_nCurSelConfig;
    
    ListView*	  m_pcFontList;
    DropdownMenu* m_pcFontConfigList;
    DropdownMenu* m_pcBitmapSizeList;
    Spinner*	  m_pcSizeSpinner;

    TextView*	  m_pcTestView;
    Button*	  m_pcRescanBut;
    Button*	  m_pcOkBut;
    Button*	  m_pcCancelBut;
};

#endif // __F_FONTPANEL_H__
