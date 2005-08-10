#include "application.h"
#include <util/catalog.h>

using namespace os;

RunApplication::RunApplication() : Application("application/x-vnd.syllable.Run")
{	
	SetCatalog("Launch.catalog");

	pcMainWindow = new MainWindow();
	pcMainWindow->CenterInScreen();
	pcMainWindow->Show();
	pcMainWindow->MakeFocus(true);
}


