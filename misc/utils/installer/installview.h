#ifndef __F_INSTALLVIEW_H__
#define __F_INSTALLVIEW_H__

#include <sys/types.h>
#include <gui/layoutview.h>


namespace os {
    class Button;
    class RadioButton;
    class FrameView;
    class TextView;
    class DropdownMenu;
    class FileRequester;
};

class InstallView : public os::LayoutView
{
public:
    InstallView( const std::string& cName );
    ~InstallView();

    virtual void AttachedToWindow();
    virtual void HandleMessage( os::Message* pcMessage );

private:
    void InitDstPath( const char* pzPath );

    os::FrameView*	m_pcSourceFrame;
    os::RadioButton*	m_pcSrcFloppy;
    os::RadioButton*	m_pcSrcFile;
    os::TextView*	m_pcSrcPath;
    os::Button*		m_pcBrowseSrcBut;
    os::DropdownMenu*	m_pcDstPath;
    os::Button*		m_pcBrowseDstBut;
    
    
    os::FrameView*	m_pcDestFrame;
    os::Button*		m_pcQuitBut;
    os::Button*		m_pcTerminalBut;
    os::Button*		m_pcInstallBut;

    os::FileRequester*	m_pcSrcPathRequester;
    os::FileRequester*	m_pcDstPathRequester;
};


#endif // __F_INSTALLVIEW_H__
