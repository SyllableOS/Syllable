#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

#include "include.h"
#include "imagebutton.h"

static os::String pzLogin;
static os::String cPassword;
static bool g_bRun = true;
using namespace os;

class LoginView : public View
{
public:
    LoginView( const Rect& cFrame );
    ~LoginView();

    virtual void 	AllAttached();
    virtual void 	FrameSized( const Point& cDelta );
    void			SetOptions(bool bOpt);
	bool			GetOptions(){return bOptions;}

	TextView*		m_pcNameView;
    StringView*		m_pcNameLabel;
    TextView*		m_pcPasswordView;
    StringView*		m_pcPasswordLabel;
    ColorButton*	m_pcOkBut;
	ColorButton*	m_pcOptionsBut;
	LoginImageButton*	m_pcShutdownBut;
	StringView*		m_pcLaunchString;
	TextView*		m_pcLaunchText;


private:
    void 			Layout();
	void			SetTabs();

	virtual void 	Paint(const Rect& cUpdate);
    bool          	Read();
    void          	LoadImages();
    
	bool			bOptions;
	BitmapImage*    pcLoginImage;
	BitmapImage*	pcShutdownImage;
};

class LoginWindow : public Window
{
public:
    LoginWindow( const Rect& cFrame );
    ~LoginWindow();
    
	virtual bool	OkToQuit()
    {
        Application::GetInstance()->PostMessage(M_QUIT );
        return( true );
    }
    
	virtual void	HandleMessage( Message* pcMessage );
    LoginView* 		m_pcView;
private:
	void       		Authorize(const char* pzLoginName);
	void			LaunchShutdown();
	void			LaunchOptions();
};

#endif
