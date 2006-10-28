#include "application.h"
#include "commonfuncs.h"

App::App():os::Application( "application/x-vnd.syllable-login_application" )
{
	SetCatalog("dlogin.catalog");

	mkdir("/system/icons/users",S_IRWXU | S_IRWXG | S_IRWXO);
	SetPublic(true);

	IPoint cPoint = GetResolution();
	m_pcMainWindow = new MainWindow(Rect(0,0,cPoint.x,cPoint.y));
	m_pcMainWindow->Init();
	m_pcMainWindow->Show();
	m_pcMainWindow->MakeFocus();
}

void App::HandleMessage(Message* pcMessage)
{
	switch (pcMessage->GetCode())
	{
		case M_NO_REPLY:
			m_pcMainWindow->Refresh();
			break;
		default:
			Application::HandleMessage(pcMessage);
	}
}
