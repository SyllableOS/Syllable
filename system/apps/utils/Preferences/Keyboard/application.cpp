#include "application.h"
#include "mainwindow.h"

App::App() : os::Application( "application/x-vnd-KeyboardPreferences" )
{
	SetCatalog("Keyboard.catalog");

	m_pcMainWindow = new MainWindow();
	m_pcMainWindow->CenterInScreen();
	m_pcMainWindow->Show();
	m_pcMainWindow->MakeFocus();
}

