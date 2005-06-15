#ifndef __MAINWIN_H__
#define __MAINWIN_H__

#include <gui/window.h>
#include <gui/menu.h>
#include <gui/statusbar.h>
#include <util/message.h>
#include "charmap.h"

class MainWin : public os::Window
{
public:
    MainWin( const os::Rect& cRect );
    ~MainWin();

	void HandleMessage( os::Message *pcMsg );

	bool OkToQuit();
private:
	os::Menu* _CreateMenuBar();
	CharMapView*		m_pcCharMap;
    os::StatusBar* 		m_pcStatusBar;
    os::ScrollBar*		m_pcScrollBar;
	os::String			m_cFamily;
	os::String			m_cStyle;
	float				m_vSize;
	uint32				m_nFlags;
};

#endif // __MAINWIN_H__
