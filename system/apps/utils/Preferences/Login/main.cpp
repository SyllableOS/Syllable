#include <stdlib.h>
#include <gui/rect.h>
#include <util/application.h>

#include <login_window.h>

using namespace os;

class LoginApplication : public Application
{
	public:
		LoginApplication();

	private:
		LoginWindow *m_pcLoginWindow;
};

LoginApplication::LoginApplication() : Application( "application/x-vnd-LoginPreferences" )
{
	SetCatalog("Login.catalog");

	m_pcLoginWindow = new LoginWindow( Rect( 0, 0, 399, 449 ) );

	m_pcLoginWindow->CenterInScreen();
	m_pcLoginWindow->Show();
	m_pcLoginWindow->MakeFocus();
}

int main( int argc, char **argv )
{
	LoginApplication *pcApplication = new LoginApplication();
	pcApplication->Run();

	return EXIT_SUCCESS;
}

