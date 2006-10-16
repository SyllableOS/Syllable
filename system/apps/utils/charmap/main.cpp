#include <gui/window.h>
#include <gui/requesters.h>
#include <util/application.h>

#include "mainwin.h"

using namespace os;

class MyApp : public Application
{
    public:
    MyApp() : Application( "application/x-vnd.digitaliz-Charmap" ) {
		SetCatalog("CharMap.catalog");
    	m_pcWindow = new MainWin( Rect( 0, 0, 500, 250 ) );
    	m_pcWindow->CenterInScreen();
    	m_pcWindow->Show();
    }

    ~MyApp() {
    }

    private:
    Window *m_pcWindow;
};

int main(void)
{
    MyApp *pcApp = new MyApp();

    pcApp->Run();
}
