#ifndef _SWINDOW_H_
#define _SWINDOW_H_

#include <gui/window.h>
#include <gui/tableview.h>
#include <gui/stringview.h>
#include <gui/dropdownmenu.h>
#include <gui/checkbox.h>
#include <gui/button.h>


#include "settings.h"

using namespace os;


class SettingsWindow:public Window
{
  public:
    SettingsWindow( const Rect & cFrame, TermSettings * pcSettings,
        const String &cName, const String &cTitle, uint32 nFlags = 0 );
     ~SettingsWindow();

    bool OkToQuit();
    void HandleMessage( Message * pcMsg );

  private:
    void ShowSettings();
    void ApplySettings();

    TermSettings m_cTempSettings;
    TermSettings *m_pcDestSettings;

    TableView *m_pcColorsTable;
    TableView *m_pcLayoutTable;
    TableView *m_pcButtonsTable;

    StringView *m_pcNormalLabel;
    StringView *m_pcSelectedLabel;
    StringView *m_pcTextLabel;
    StringView *m_pcBackgroundLabel;
    StringView *m_pcNoticeLabel;
    DropdownMenu *m_pcNormalTextList;
    DropdownMenu *m_pcNormalBackgroundList;
    DropdownMenu *m_pcSelectedTextList;
    DropdownMenu *m_pcSelectedBackgroundList;

    CheckBox *m_pcIBeamCheck;

    Button *m_pcApplyButton;
    Button *m_pcSaveButton;
    Button *m_pcCancelButton;
    Button *m_pcRevertButton;
};


#endif // _SWINDOW_H_
