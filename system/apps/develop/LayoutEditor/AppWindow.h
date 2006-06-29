#ifndef APPWINDOW_H
#define APPWINDOW_H

#include <util/application.h>
#include <gui/window.h>
#include <gui/layoutview.h>
#include <util/message.h>

class MainWindow;

class AppWindow : public os::Window
{
public:
	AppWindow( MainWindow* pcMain );
	void HandleMessage( os::Message* );
	os::VLayoutNode* GetRootNode() { return( m_pcRoot ); }
	void ReLayout( os::LayoutNode* pcNode );
	void ReLayout();
private:
	bool OkToQuit();
	MainWindow* m_pcMain;
	os::LayoutView* m_pcLayoutView;
	os::VLayoutNode* m_pcRoot;
};

#endif






