#ifndef __F_FILEDIALOG_H__
#define __F_FILEDIALOG_H__

#include <gui/window.h>
#include <gui/layoutview.h>

#include <util/message.h>
#include <util/messenger.h>


namespace os {
    class Button;
    class CheckBox;
    class TextView;
}

class FindDialogView : public os::LayoutView
{
public:
    FindDialogView( const os::Rect& cFrame, const os::Messenger& cMsgTarget, const os::Message& cSearchMsg, const os::Message& cCloseMsg );
    ~FindDialogView();

    virtual void AllAttached();
    virtual void KeyDown( const char* pzString, const char* pzRawString, uint32 nQualifiers );
    virtual void HandleMessage( os::Message* pcMessage );
    
private:
    os::TextView* m_pcSearchStr;
    os::Button*	  m_pcSearchBut;
    os::Button*   m_pcContinueBut;
    os::Button*   m_pcCloseBut;
    os::CheckBox* m_pcCaseSensCB;
    
    os::Messenger m_cMsgTarget;
    os::Message   m_cSearchMsg;
    os::Message   m_cCloseMsg;
};


class FindDialog : public os::Window
{
public:
    FindDialog( const os::Messenger& cMsgTarget, const os::Message& cSearchMsg, const os::Message& cCloseMsg );
    ~FindDialog();
private:
    FindDialogView* m_pcTopView;
};



#endif // __F_FILEDIALOG_H__
