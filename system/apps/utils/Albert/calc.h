
#ifndef CALC_H
#define CALC_H

#include <gui/window.h>
#include <util/message.h>
#include <util/settings.h>

using namespace os;

#include "calculator.h"
#include "calcbuttons.h"
#include "remsa.h"
#include "base-n.h"
#include "postoffice.h"

class CalcDisplay;

class CalcView: public View {
	public:
	CalcView(const Rect & r);
	~CalcView();
	virtual void KeyDown( const char * pzString,  const char * pzRawString,  uint32 nQualifiers );
	virtual void KeyUp( const char * pzString,  const char * pzRawString,  uint32 nQualifiers );
	virtual void Activated( bool bIsActive );
	virtual void WindowActivated( bool bIsActive );
};

class CalcWindow: public Window {
	public:
	CalcWindow(const Rect & r);
	~CalcWindow();
	bool OkToQuit();

	void UpdateDisplay(long double value);
	long double ConvertDisplay(void);
	void AddToRoll(char *fmt, char *op = 0, long double val = 0);

	void HandleMessage(Message *);

	void BuildMenus(void);
	void ShowAbout(void);
	void ShowKeys(void);

	private:
	Calculator				calc;
	char					bfr[256];
	bool					newentry;
	CalcDisplay			*display;
	bool					inv;
	bool					hyp;
	SimpleCalcButtons		*m_CalcButtons;
	uint8					m_Base;
	char					m_LastOp[16];
	CalcView				*m_CalcView;
	Settings				*m_Settings;
	Menu*					m_pcMenuBar;
};

#endif




