#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gui/window.h>
#include <gui/layoutview.h>
#include <gui/tabview.h>
#include <gui/frameview.h>
#include <gui/checkbox.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <gui/stringview.h>
#include <gui/slider.h>
#include <gui/button.h>
#include <gui/imagebutton.h>
#include <gui/requesters.h>
#include <util/application.h>
#include <util/message.h>
#include <util/resources.h>
#include <storage/directory.h>
#include <storage/nodemonitor.h>
#include <appserver/keymap.h>
using namespace os;


#include <util/locale.h>
#include <util/settings.h>
#include <storage/file.h>
#include <iostream>
#include <sys/stat.h>


class LanguageInfo {
	public:
	LanguageInfo( char *s )
	{
		char bfr[256], *b;
		uint i, j;

		for( j = 0; j < 3 && *s; j++ ) {
			for( b = bfr, i = 0; i < sizeof( bfr ) && *s && *s != '\t'; i++ )
				*b++ = *s++;
			*b = 0;
			for( ; *s == '\t' ; s++ );

			m_cInfo[j] = bfr;
		}

		m_bValid = false;		
		if( m_cInfo[0][0] != '#' && GetName() != "" && GetAlias() != "" && GetCode() != "" ) {
			m_bValid = true;
		}
	}

	bool IsValid() { return m_bValid; }
	os::String& GetName()		{ return m_cInfo[0]; }
	os::String& GetAlias()		{ return m_cInfo[1]; }
	os::String& GetCode()		{ return m_cInfo[2]; }
	private:
	os::String m_cInfo[3];
	bool m_bValid;
};


class MainWindow : public os::Window
{
public:
	MainWindow();
	void ShowData();
	void SetCurrent();
	void LoadLanguages();
	void Apply();
	void Undo();
	void Default();
	void HandleMessage( os::Message* );
private:
	void LoadDatabase();
	bool OkToQuit();
	#include "mainwindowLayout.h"
	NodeMonitor* m_pcMonitor;

	os::String cPrimaryLanguage;
	os::String cKeymap;
	int iDelay, iRepeat, iDelay2, iRepeat2;
	int iOrigRow, iOrigRow2;
	std::vector<LanguageInfo> m_cDatabase;
};

#endif




