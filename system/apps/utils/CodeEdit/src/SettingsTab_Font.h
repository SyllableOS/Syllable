#ifndef  _SETTINGSFONT_H
#define _SETTINGSFONT_H

namespace os
{
class Button;
class ListView;
class TextView;
class DropdownMenu;
class StringView;
class CheckBox;
class Font;
};

#include <gui/view.h>
#include <gui/font.h>
#include <codeview/codeview.h>
#include <vector>
#include "EditWin.h"
#include "App.h"

using namespace cv;

class SettingsTab_Fonts : public os::View
{
private:
    App* app;

    enum { ID_CHANGE_FORMAT, ID_VIEW_FONT, ID_CHANGE_FONT_SIZE};

    os::Button *pcOk;
    os::Button *bCancel;
    os::DropdownMenu *pcFonts;
    //os::ListView* pcFonts;
    os::StringView *pcSSize, *pcSFonts;
    os::DropdownMenu *pcDSize;
    os::CheckBox *pcSheared, *pcAliased;

    void _Init();
    struct ConfigNode
    {
        std::string	    m_cName;
        os::font_properties m_sConfig;
        os::font_properties m_sOrigConfig;
        bool		    m_bModified;
    };

    std::vector<ConfigNode> m_cConfigList;
public:
    SettingsTab_Fonts(App*,EditWin*);

    void apply();
    void getFonts();
    virtual void HandleMessage(os::Message*);
    virtual void AllAttached();
};

#endif






