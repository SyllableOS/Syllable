/*
 * Albert 0.5 * Copyright (C)2002-2004 Henrik Isaksson
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "resources/Calculator.h"

#include <gui/window.h>
#include <gui/view.h>
#include <gui/rect.h>
#include <gui/button.h>
#include <gui/layoutview.h>
#include <gui/listview.h>
#include <gui/textview.h>
#include <gui/tableview.h>
#include <gui/menu.h>
#include <gui/requesters.h>
#include <util/message.h>
#include <util/application.h>

using namespace os;

#include "calculator.h"
#include "calcbuttons.h"
#include "calcdisplay.h"
#include "calc.h"
#include "remsa.h"
#include "base-n.h"
#include "aboutwin.h"
#include "crect.h"

#include "postoffice.h"

#define __USE_ISOC9X
#include <stdlib.h>
#include <math.h>
#include <iostream>

using namespace std;

class CalcApp:public Application
{
	public:
	CalcApp();
	~CalcApp();

	CalcWindow *win;

	private:
};

enum MenuIDs {
	ID_CONSTANT_C = 256,
	ID_CONSTANT_PI,
	ID_CONSTANT_E,
	ID_CONSTANT_ELECTRON,
	ID_CONSTANT_GRAV,
	ID_CONSTANT_PLANCK,
	ID_CONSTANT_BOLTZMANN,
	ID_CONSTANT_AVOGADRO,
	ID_CONSTANT_GAS,
	ID_CONSTANT_MASS_ELECTRON,
	ID_CONSTANT_MASS_PROTON,
	ID_CONSTANT_MASS_NEUTRON,
	ID_CONSTANT_PERME,
	ID_CONSTANT_PERMI,

	ID_WINDOW_PAPER,
	ID_WINDOW_BASEN,

	ID_BASE_2,
	ID_BASE_8,
	ID_BASE_10,
	ID_BASE_16,

	ID_SHOW_KEYS
};

#define ALBERT_VERSION "0.5"

void CalcWindow::BuildMenus(void)
{
	// Create a menu bar

	Rect sMenuBounds = GetBounds();
	sMenuBounds.bottom = 0;

	m_pcMenuBar = new Menu(sMenuBounds, "main_menu", ITEMS_IN_ROW, CF_FOLLOW_LEFT | CF_FOLLOW_RIGHT, WID_FULL_UPDATE_ON_H_RESIZE);

	// Create the menus within the bar

	Menu* pcFileMenu = new Menu(Rect(0,0,1,1),MSG_MAINWND_MENU_FILE, ITEMS_IN_COLUMN);
	pcFileMenu->AddItem(MSG_MAINWND_MENU_FILE_CLEARALL, new Message('a'), "CTRL+C");
//	pcFileMenu->AddItem(new MenuSeparator);
	pcFileMenu->AddItem(MSG_MAINWND_MENU_FILE_EXIT, new Message('q'), "CTRL+Q" );

	m_pcMenuBar->AddItem(pcFileMenu);

	Menu* pcBaseMenu = new Menu(Rect(0,0,1,1),MSG_MAINWND_MENU_BASE, ITEMS_IN_COLUMN);
	pcBaseMenu->AddItem(MSG_MAINWND_MENU_BASE_BINARY, new Message(ID_BASE_2), "CTRL+B" );
	pcBaseMenu->AddItem(MSG_MAINWND_MENU_BASE_OCTAL, new Message(ID_BASE_8), "CTRL+O");
	pcBaseMenu->AddItem(MSG_MAINWND_MENU_BASE_DECIMAL, new Message(ID_BASE_10), "CTRL+D");
	pcBaseMenu->AddItem(MSG_MAINWND_MENU_BASE_HEXADECIMAL, new Message(ID_BASE_16), "CTRL+H");


	m_pcMenuBar->AddItem(pcBaseMenu);

	Menu* pcConstMenu = new Menu(Rect(0,0,1,1),MSG_MAINWND_MENU_CONSTANTS, ITEMS_IN_COLUMN);
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_PI, new Message(ID_CONSTANT_PI));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_E, new Message(ID_CONSTANT_E));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_C, new Message(ID_CONSTANT_C));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_ELECTRON, new Message(ID_CONSTANT_ELECTRON));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_GRAV, new Message(ID_CONSTANT_GRAV));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_PLANCK, new Message(ID_CONSTANT_PLANCK));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_BOLTZMANN, new Message(ID_CONSTANT_BOLTZMANN));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_AVOGADRO, new Message(ID_CONSTANT_AVOGADRO));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_GAS, new Message(ID_CONSTANT_GAS));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_MASSELECTRON, new Message(ID_CONSTANT_MASS_ELECTRON));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_MASSPROTON, new Message(ID_CONSTANT_MASS_PROTON));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_MASSNEUTRON, new Message(ID_CONSTANT_MASS_NEUTRON));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_PERME, new Message(ID_CONSTANT_PERME));
	pcConstMenu->AddItem(MSG_MAINWND_MENU_CONSTANTS_PERMI, new Message(ID_CONSTANT_PERMI));

	m_pcMenuBar->AddItem(pcConstMenu);

	Menu* pcWinMenu = new Menu(Rect(0,0,1,1),MSG_MAINWND_MENU_WINDOWS, ITEMS_IN_COLUMN);
	pcWinMenu->AddItem(MSG_MAINWND_MENU_WINDOWS_PAPERROLL, new Message(ID_WINDOW_PAPER), "CTRL+P");
	pcWinMenu->AddItem(MSG_MAINWND_MENU_WINDOWS_BASEN, new Message(ID_WINDOW_BASEN), "CTRL+N");

	m_pcMenuBar->AddItem(pcWinMenu);

	Menu* pcHelpMenu = new Menu(Rect(0,0,1,1),MSG_MAINWND_MENU_HELP, ITEMS_IN_COLUMN);
	pcHelpMenu->AddItem(MSG_MAINWND_MENU_HELP_ABOUT, new Message('?'));
	pcHelpMenu->AddItem(MSG_MAINWND_MENU_HELP_KEYBOARD, new Message(ID_SHOW_KEYS));

	m_pcMenuBar->AddItem(pcHelpMenu);


	m_pcMenuBar->SetTargetForItems(this);

	// Add the menubar to the window

	m_CalcView->AddChild(m_pcMenuBar);

	sMenuBounds.bottom = sMenuBounds.top + m_pcMenuBar->GetPreferredSize(false).y;
	m_pcMenuBar->SetFrame( sMenuBounds );
}

void CalcWindow::UpdateDisplay(long double value)
{
	Message *msg = new Message(1);
	msg->AddInt64("value", (int64)value);

	Mail("Base-N", msg);

	switch(m_Base) {
		case 16:
			sprintf(bfr, "%Lx", (int64)value);
			break;
		case 8:
			sprintf(bfr, "%Lo", (int64)value);
			break;
		case 2:
			IToBin((int64)value, bfr);
			break;
		default:
			sprintf(bfr, "%.16g", (double)value);
	}
	display->Set(bfr);
}

long double CalcWindow::ConvertDisplay(void)
{
	int64 val;

	switch(m_Base) {
		case 16:
			sscanf(bfr, "%Lx", &val);
			return (long double)val;
			break;
		case 8:
			sscanf(bfr, "%Lo", &val);
			return (long double)val;
			break;
		case 2:
			return (long double)BinToI(bfr);
			break;
		default:
			return (long double)strtod((const char *)bfr, NULL);
	}
}

void CalcWindow::AddToRoll(char *fmt, char *op, long double val)
{
	char b[64];
	Message *msg = new Message(1);

	sprintf(b, fmt, op, (double)val);
	msg->AddString("text", b);

	Mail("PaperRoll", msg);
}

void CalcWindow::HandleMessage(Message *msg)
{
	long double tmp = ConvertDisplay();
	int code;

	#define ADDTOROLL(op)		AddToRoll("%6s %12.2lf\n", m_LastOp, tmp);\
									strcpy(m_LastOp, op);

	#define BINARY_OP(op,func)	ADDTOROLL(op);\
									tmp = calc.func(tmp);\
									UpdateDisplay(tmp);\
									newentry = true;

	#define UNARY_OP(op, func)	tmp = calc.func(tmp);\
									UpdateDisplay(tmp);\
									AddToRoll("%6s\n", op);\
									newentry = true;

	#define SET_CONSTANT(c)		UpdateDisplay(c);\
									newentry = true;

	#define SET_BASE(n, func)	m_Base = n;\
									m_CalcButtons->func();\
									UpdateDisplay(tmp);

	#define ADD_DIGIT(d)			{ char tmpbfr[2];\
									tmpbfr[0] = d;\
									tmpbfr[1] = 0;\
									if(newentry || (bfr[0] == '0' && bfr[1] == '\0'))\
										strcpy(bfr, tmpbfr);\
									else\
										strcat(bfr, tmpbfr);\
									display->Set(bfr);\
									newentry = false; }


	switch((code = msg->GetCode())) {
		case 'q':
			OkToQuit();
			break;
		default:
			if((code >= 'A' && code <= 'F') && m_Base >= 16)
				ADD_DIGIT(msg->GetCode());
			if((code >= '8' && code <= '9') && m_Base >= 10)
				ADD_DIGIT(msg->GetCode());
			if((code >= '2' && code <= '7') && m_Base >= 8)
				ADD_DIGIT(msg->GetCode());
			if((code >= '0' && code <= '1') && m_Base >= 2)
				ADD_DIGIT(msg->GetCode());
			break;
		case CB_CLEAR:
			strcpy(bfr, "0");
			display->Set(bfr);
			newentry = true;
			break;
		case CB_CLEAR_ALL:
			strcpy(bfr, "0");
			display->Set(bfr);
			display->SetMem(false);
			display->SetPar(0);
			calc.Clear();
			newentry = true;
			AddToRoll("...................\n",0,0);
			break;
		case CB_EQUAL:
			AddToRoll("%6s %12.2lf\n", m_LastOp, tmp);
			strcpy(m_LastOp, "");
			tmp = calc.Equals(ConvertDisplay());
			UpdateDisplay(tmp);
			newentry = true;
			AddToRoll("%6s %12.2lf\n\n", " =", tmp);
			break;
		case CB_OPEN_PAR:
			tmp = calc.OpenParen(ConvertDisplay());
			display->SetPar(calc.ParDepth());
			cout << calc.ParDepth() << endl;
			UpdateDisplay(tmp);
			newentry = true;
			break;
		case CB_CLOSE_PAR:
			tmp = calc.CloseParen(ConvertDisplay());
			display->SetPar(calc.ParDepth());
			cout << calc.ParDepth() << endl;
			UpdateDisplay(tmp);
			newentry = true;
			break;
		case CB_INV:
			inv = !inv;
			display->SetInv(inv);
			calc.Inv(inv);
			break;
		case CB_HYP:
			hyp = !hyp;
			display->SetHyp(hyp);
			calc.Hyp(hyp);
			break;
		case CB_POINT:
			if(m_Base == 10) {
				if(newentry)
					break;

				bool haspoint = false;
				int i;

				for(i=0;bfr[i];i++)
					if(bfr[i] == '.')
						haspoint = true;

				if(haspoint)
					break;

				strcat(bfr, ".");
				display->Set(bfr);
			}
			break;
		case CB_BACKSPACE:
			if(!newentry) {
				int len = strlen(bfr);
				if(len>1)
					bfr[len-1] = 0;
				else
					bfr[0] = '0';
				display->Set(bfr);
			}
			break;
		case CB_PLUS:		BINARY_OP("+", Add); 	break;
		case CB_MUL:		BINARY_OP("*", Mul); 	break;
		case CB_MINUS:	BINARY_OP("-", Sub);		break;
		case CB_DIV:		BINARY_OP("/", Div);		break;
		case CB_AND:		BINARY_OP("AND", And);	break;
		case CB_OR:		BINARY_OP("OR", Or);		break;
		case CB_XOR:		BINARY_OP("XOR", Xor);	break;
		case CB_POWX:		BINARY_OP("x^y", PowX);	break;
		case CB_SIN:		UNARY_OP("sin", Sin);	break;
		case CB_COS:		UNARY_OP("cos", Cos);	break;
		case CB_TAN:		UNARY_OP("tan", Tan);	break;
		case CB_LOG:		UNARY_OP("log", Log);	break;
		case CB_LN:		UNARY_OP("ln", Ln);		break;
		case CB_LOG2:		UNARY_OP("log2", Log2);	break;
		case CB_SIGN:		UNARY_OP("+/-", Neg);	break;
		case CB_NOT:		UNARY_OP("NOT", Not);	break;
		case CB_INT:		UNARY_OP("INT", Int);	break;
		case CB_SQRT:		UNARY_OP("SQRT", Sqrt);	break;
		case CB_POW2:		UNARY_OP("x^2", Pow2);	break;
		case CB_POW3:		UNARY_OP("x^3", Pow3);	break;
		case CB_POWE:		UNARY_OP("e^x", PowE);	break;
		case CB_CLEAR_MEM:
			calc.MClear(0);
			display->SetMem(false);
			break;
		case CB_READ_MEM:
			UpdateDisplay(calc.MRead(0));
			newentry = true;
			break;
		case CB_ADD_MEM:
			UpdateDisplay(tmp = calc.MPlus(ConvertDisplay()));
			if(tmp != 0)
				display->SetMem(true);
			else
				display->SetMem(false);
			newentry = true;
			break;
		case CB_SUB_MEM:
			UpdateDisplay(tmp = calc.MMinus(ConvertDisplay()));
			if(tmp != 0)
				display->SetMem(true);
			else
				display->SetMem(false);
			newentry = true;
			break;
		case '?':
			ShowAbout();
			break;
		case ID_SHOW_KEYS:
			ShowKeys();
			break;
		case ID_CONSTANT_E:				SET_CONSTANT(M_E);				break;
		case ID_CONSTANT_PI:				SET_CONSTANT(M_PI);				break;
		case ID_CONSTANT_C:				SET_CONSTANT(2.99792458E+8);	break;
		case ID_CONSTANT_ELECTRON:		SET_CONSTANT(1.60217733E-19);	break;
		case ID_CONSTANT_GRAV:			SET_CONSTANT(6.67259E-11);		break;
		case ID_CONSTANT_PLANCK:			SET_CONSTANT(6.6260755E-34);	break;
		case ID_CONSTANT_BOLTZMANN:		SET_CONSTANT(1.380658E-23);		break;
		case ID_CONSTANT_AVOGADRO:		SET_CONSTANT(6.0221367E+23);	break;
		case ID_CONSTANT_GAS:			SET_CONSTANT(8.314510);			break;
		case ID_CONSTANT_MASS_ELECTRON:	SET_CONSTANT(9.1093897E-31);	break;
		case ID_CONSTANT_MASS_PROTON:	SET_CONSTANT(1.6726231E-27);	break;
		case ID_CONSTANT_MASS_NEUTRON:	SET_CONSTANT(1.6749286E-27);	break;
		case ID_CONSTANT_PERME:			SET_CONSTANT(4E-7 * M_PI);		break;
		case ID_CONSTANT_PERMI:			SET_CONSTANT(1/(4E-7 * M_PI *
										2.99792458E+8 * 2.99792458E+8));	break;
		case ID_WINDOW_PAPER:
			/*{
				Rect pos = GetFrame();
				pos.left = pos.right+20;
				pos.right += 200;
				PaperRoll->SetFrame(pos);
				PaperRoll->Show();
			}*/
			{
				Rect pos = GetFrame();
				pos.left = pos.right+20;
				pos.right += 200;
				Remsa *pr = new Remsa(pos);
				pr->Show();
			}
			break;
		case ID_WINDOW_BASEN:
			{
				Rect pos = GetFrame();
				pos.left = pos.right+20;
				pos.right += 200;
				(new BaseN(pos))->Show();
			}
			break;

		case ID_BASE_2:	SET_BASE(2, SetBinary);			break;
		case ID_BASE_8:	SET_BASE(8, SetOctal);			break;
		case ID_BASE_10:	SET_BASE(10, SetDecimal);		break;
		case ID_BASE_16:	SET_BASE(16, SetHexadecimal);	break;
	}
}

void CalcWindow::ShowAbout(void)
{
/*	Alert* sAbout = new Alert("About Albert " ALBERT_VERSION, "Albert " ALBERT_VERSION
		"\n\nSientific calculator for AtheOS\nCopyright (C)2001 Henrik Isaksson", 0x00, "O.K", NULL);
	sAbout->Go(new Invoker);*/

	Window *w = new AboutWindow(Rect(400, 400, 700, 700));
	w->Show();

}

void CalcWindow::ShowKeys(void)
{
	Alert* sAbout = new Alert(MSG_KBDWND_TITLE,
		MSG_KBDWND_TEXTONE + "\n\n"
		"0..9 - " + MSG_KBDWND_TEXT09 + "\n"
		"A..F - " + MSG_KBDWND_TEXTAF + "\n"
		"+-*/ - " + MSG_KBDWND_TEXTASMD + "\n"
		""+ MSG_KBDWND_TEXTENTER + "\n"
		""+ MSG_KBDWND_TEXTBACKSPACE +""
		,
		0x00, MSG_KBDWND_CLOSE.c_str(), NULL);
	sAbout->Go(new Invoker);
}

CalcWindow::CalcWindow(const Rect & r)
	:Window(r, "CalcWindow", MSG_MAINWND_TITLE + " " + ALBERT_VERSION, WND_NOT_H_RESIZABLE | WND_NOT_V_RESIZABLE, CURRENT_DESKTOP)
{
	Rect bounds = GetBounds();
	m_CalcView = new CalcView(bounds);

	BuildMenus();

	bounds.top += m_pcMenuBar->GetBounds().bottom + 1;

	strcpy(bfr, "0");

	display = new CalcDisplay(Rect(bounds.left+2,bounds.top+2,bounds.right-2,bounds.top+20));
	//display->SetReadOnly(true);
	//display->setNeedsFocus(this);

	m_CalcView->AddChild(display);

	m_CalcButtons = new SimpleCalcButtons(Rect(0, bounds.top+22, bounds.right, bounds.bottom));

	m_CalcView->AddChild(m_CalcButtons);

	m_CalcView->MakeFocus(true);

	AddChild(m_CalcView);

	newentry = true;
	inv = false;
	hyp = false;
	m_LastOp[0] = '\0';
	m_Base = 10;

	AddMailbox("Main");

	MakeFocus(true);
	
	os::Resources cCol( get_image_id() );
	os::ResStream *pcStream = cCol.GetResourceStream( "icon48x48.png" );
	os::BitmapImage *pcIcon = new os::BitmapImage( pcStream );
	SetIcon( pcIcon->LockBitmap() );
	delete( pcIcon );
}

CalcWindow::~CalcWindow()
{
	RemMailbox("Main");
}

CalcView::CalcView(const Rect & r)
	:View(r,"cv")
{
}

CalcView::~CalcView()
{
}

void CalcView::Activated(bool act)
{
	if(!act)
		MakeFocus(true);
}

void CalcView::WindowActivated( bool bIsActive )
{
	MakeFocus(bIsActive);
}

void CalcView::KeyDown( const char * pzString,  const char * pzRawString,  uint32 nQualifiers )
{
	Message *msg = new Message;

	switch(toupper(pzString[0])) {
        case '0':	msg->SetCode( '0' );			break;
        case '1':	msg->SetCode( '1' );			break;
        case '2':	msg->SetCode( '2' );			break;
        case '3':	msg->SetCode( '3' );			break;
        case '4':	msg->SetCode( '4' );			break;
        case '5':	msg->SetCode( '5' );			break;
        case '6':	msg->SetCode( '6' );			break;
        case '7':	msg->SetCode( '7' );			break;
        case '8':	msg->SetCode( '8' );			break;
        case '9':	msg->SetCode( '9' );			break;

        case 'a':
        case 'A':	msg->SetCode( 'A' );			break;
        case 'b':
        case 'B':	msg->SetCode( 'B' );			break;
        case 'c':
        case 'C':	msg->SetCode( 'C' );			break;
        case 'd':
        case 'D':	msg->SetCode( 'D' );			break;
        case 'e':
        case 'E':	msg->SetCode( 'E' );			break;
        case 'f':
        case 'F':	msg->SetCode( 'F' );			break;

        case '+':	msg->SetCode( CB_PLUS );		break;
        case '-':	msg->SetCode( CB_MINUS );	break;
        case '*':	msg->SetCode( CB_MUL );		break;
        case '/':	msg->SetCode( CB_DIV );		break;

        case ',':
        case '.':	msg->SetCode( CB_POINT );	break;

        case '\b':	msg->SetCode( CB_BACKSPACE );		break;
        case '\n':	msg->SetCode( CB_EQUAL );			break;

  //      case 127:		msg->SetCode( CB_CLEAR_ALL );		break;

        default:
			Message *keymsg = ((CalcApp *)Application::GetInstance())->win->GetCurrentMessage();
			int32 z = 0;

			keymsg->FindInt32("_raw_key", &z);
        	switch(z) {
		        case 100:	msg->SetCode( '0' );			break;
		        case 88:	msg->SetCode( '1' );			break;
		        case 89:	msg->SetCode( '2' );			break;
		        case 90:	msg->SetCode( '3' );			break;
		        case 72:	msg->SetCode( '4' );			break;
		        case 73:	msg->SetCode( '5' );			break;
		        case 74:	msg->SetCode( '6' );			break;
		        case 55:	msg->SetCode( '7' );			break;
		        case 56:	msg->SetCode( '8' );			break;
		        case 57:	msg->SetCode( '9' );			break;
		        case 101:	msg->SetCode( CB_POINT );		break;
		        case 52:	msg->SetCode( CB_CLEAR_ALL );	break;
        		default:
        			//cout << z << endl;
        			View::KeyDown( pzString,  pzRawString, nQualifiers );
	        		msg->SetCode( 0 );
        	}
			break;
	}
	Mail("Main", msg);
}

void CalcView::KeyUp( const char * pzString,  const char * pzRawString,  uint32 nQualifiers )
{
	View::KeyUp( pzString,  pzRawString, nQualifiers );
}

bool CalcWindow::OkToQuit(void)
{
	Broadcast(new Message(M_QUIT));
	Application::GetInstance()->PostMessage(M_QUIT);
	return true;
}

CalcApp::CalcApp()
	:Application("application/x-vnd.digitaliz-Calculator")
{
	SetCatalog("Calculator.catalog");
	win = new CalcWindow(CRect(350,178));
	win->Show();
}

CalcApp::~CalcApp()
{
}

int main(void)
{
	CalcApp *thisApp;

	thisApp = new CalcApp;

	thisApp->Run();

	PostOffice::GetInstance()->Quit();

	return 0;
}




