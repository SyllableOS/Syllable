#ifndef LOGIN_H
#define LOGIN_H

#include "include.h"
#include "crect.h"
#include "colorbutton.h"
#include "loadbitmap.h"

using namespace os;



#define ID_OK 1
#define ID_CANCEL 2

static bool g_bRun      = true;
static bool g_bSelected = false;

static std::string g_cName;
static std::string g_cPassword;


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
        virtual bool	OkToQuit() { g_bRun = false; return( true ); }
        virtual void	HandleMessage( Message* pcMessage );
    private:
        LoginView* m_pcView;
};

#endif





