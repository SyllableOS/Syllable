#define _GNU_SOURCE

#ifndef LOGIN_H
#define LOGIN_H


#include "include.h"
#include "crect.h"
#include "colorbutton.h"
#include "loadbitmap.h"



using namespace os;

static std::string pzLogin;
static std::string cPassword;

#define ID_OK 1
#define ID_CANCEL 2
#define M_BAD_PASS 3

static bool g_bRun      = true;
//static bool g_bSelected = false;




class LoginView : public View
{
    public:
        LoginView( const Rect& cFrame );
        ~LoginView();

        virtual void AllAttached();
        virtual void FrameSized( const Point& cDelta );
        TextView*	m_pcNameView;
        StringView*	m_pcNameLabel;
        TextView*	m_pcPasswordView;
        StringView*	m_pcPasswordLabel;
        ColorButton*	m_pcOkBut;


    private:
        void Layout();
        virtual void Paint(const Rect& cUpdate);
        bool          Read();
        void          LoadImages();
        Bitmap*       pcLoginImage;
        Bitmap*		  pcAtheImage;
		 
       

};

class LoginWindow : public Window
{
    public:
        LoginWindow( const Rect& cFrame );
		 ~LoginWindow();
        virtual bool	OkToQuit() {  Application::GetInstance()->PostMessage(M_QUIT ); return( true ); }
        virtual void	HandleMessage( Message* pcMessage );
		 LoginView* 	m_pcView;
    private:
        
		 void       Authorize(const char* pzLoginName);
};




class LoginApp : public Application
{
	public:
		LoginApp();
	
	private:
		LoginWindow* pcWindow;
};

#endif



























