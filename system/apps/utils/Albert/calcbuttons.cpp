/*
 * Albert 0.4 * Copyright (C)2002 Henrik Isaksson
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

#include "calcbuttons.h"
#include "togglebutton.h"

SimpleCalcButtons::SimpleCalcButtons(const Rect &r)
	:View(r, "SimpleCalcButtons",CF_FOLLOW_LEFT|CF_FOLLOW_TOP,WID_CLEAR_BACKGROUND)
{
	m_NWidth = 10;
	m_NHeight = 5;
	m_Spacing = 2;

	m_Buttons = new Button *[m_NWidth*m_NHeight];

	float bheight = (int)(r.Height()/m_NHeight+0.5);
	float bwidth = (int)(r.Width()/m_NWidth+0.5);

	#define BUTTON(y, x, text, id)		\
			m_Buttons[x*m_NHeight+y] = new Button( \
					Rect(x*bwidth+m_Spacing, y*bheight+m_Spacing, \
						(x+1)*bwidth-m_Spacing, (y+1)*bheight-m_Spacing), \
					text, text, new Message(id)); \
			AddChild(m_Buttons[x*m_NHeight+y]);

	#define TBUTTON(y, x, text, id)		\
			m_Buttons[x*m_NHeight+y] = new ToggleButton( \
					Rect(x*bwidth+m_Spacing, y*bheight+m_Spacing, \
						(x+1)*bwidth-m_Spacing, (y+1)*bheight-m_Spacing), \
						text, text, new Message(id)); \
			AddChild(m_Buttons[x*m_NHeight+y]);

	BUTTON(0, 0, "7", CB_7);
	BUTTON(0, 1, "8", CB_8);
	BUTTON(0, 2, "9", CB_9);
	BUTTON(0, 3, "/", CB_DIV);
	BUTTON(0, 4, "AC", CB_CLEAR_ALL);
	BUTTON(0, 5, "MC", CB_CLEAR_MEM);
	BUTTON(0, 6, "(", CB_OPEN_PAR);
	BUTTON(0, 7, "sin", CB_SIN);
	BUTTON(0, 8, "x^2", CB_POW2);
	BUTTON(0, 9, "and", CB_AND);

	BUTTON(1, 0, "4", CB_4);
	BUTTON(1, 1, "5", CB_5);
	BUTTON(1, 2, "6", CB_6);
	BUTTON(1, 3, "*", CB_MUL);
	BUTTON(1, 4, "C", CB_CLEAR);
	BUTTON(1, 5, "MR", CB_READ_MEM);
	BUTTON(1, 6, ")", CB_CLOSE_PAR);
	BUTTON(1, 7, "cos", CB_COS);
	BUTTON(1, 8, "x^3", CB_POW3);
	BUTTON(1, 9, "or", CB_OR);

	BUTTON(2, 0, "1", CB_1);
	BUTTON(2, 1, "2", CB_2);
	BUTTON(2, 2, "3", CB_3);
	BUTTON(2, 3, "-", CB_MINUS);
	BUTTON(2, 4, "<-", CB_BACKSPACE);
	BUTTON(2, 5, "M+", CB_ADD_MEM);
	BUTTON(2, 6, "log", CB_LOG);
	BUTTON(2, 7, "tan", CB_TAN);
	BUTTON(2, 8, "x^y", CB_POWX);
	BUTTON(2, 9, "xor", CB_XOR);

	BUTTON(3, 0, "0", CB_0);
	BUTTON(3, 1, ".", CB_POINT);
	BUTTON(3, 2, "+/-", CB_SIGN);
	BUTTON(3, 3, "+", CB_PLUS);
	BUTTON(3, 4, "=", CB_EQUAL);
	BUTTON(3, 5, "M-", CB_SUB_MEM);
	BUTTON(3, 6, "ln", CB_LN);
	TBUTTON(3, 7, "inv", CB_INV);
	BUTTON(3, 8, "e^x", CB_POWE);
	BUTTON(3, 9, "not", CB_NOT);

	BUTTON(4, 0, "A", CB_A);
	BUTTON(4, 1, "B", CB_B);
	BUTTON(4, 2, "C", CB_C);
	BUTTON(4, 3, "D", CB_D);
	BUTTON(4, 4, "E", CB_E);
	BUTTON(4, 5, "F", CB_F);
	BUTTON(4, 6, "lg2", CB_LOG2);
	TBUTTON(4, 7, "hyp", CB_HYP);
	BUTTON(4, 8, "sqrt", CB_SQRT);
	BUTTON(4, 9, "int", CB_INT);

	SetDecimal();
}

SimpleCalcButtons::~SimpleCalcButtons()
{
	int x, y;

	for(x = 0; x < m_NWidth; x++) {
		for(y = 0; y < m_NHeight; y++) {
			delete m_Buttons[x*m_NHeight+y];
		}
	}
	delete []m_Buttons;
}

void SimpleCalcButtons::SetBinary(void)
{
	m_Buttons[0]->SetEnable(false);
	m_Buttons[1]->SetEnable(false);
	m_Buttons[1*m_NHeight+0]->SetEnable(false);
	m_Buttons[1*m_NHeight+1]->SetEnable(false);
	m_Buttons[1*m_NHeight+2]->SetEnable(false);
	m_Buttons[2*m_NHeight+0]->SetEnable(false);
	m_Buttons[2*m_NHeight+1]->SetEnable(false);
	m_Buttons[2*m_NHeight+2]->SetEnable(false);
	m_Buttons[1*m_NHeight+3]->SetEnable(false);

	int i;
	for(i=0;i<6;i++)
		m_Buttons[i*m_NHeight+4]->SetEnable(false);
}

void SimpleCalcButtons::SetHexadecimal(void)
{
	m_Buttons[0]->SetEnable(true);
	m_Buttons[1]->SetEnable(true);
	m_Buttons[1*m_NHeight+0]->SetEnable(true);
	m_Buttons[1*m_NHeight+1]->SetEnable(true);
	m_Buttons[1*m_NHeight+2]->SetEnable(true);
	m_Buttons[2*m_NHeight+0]->SetEnable(true);
	m_Buttons[2*m_NHeight+1]->SetEnable(true);
	m_Buttons[2*m_NHeight+2]->SetEnable(true);
	m_Buttons[1*m_NHeight+3]->SetEnable(false);

	int i;
	for(i=0;i<6;i++)
		m_Buttons[i*m_NHeight+4]->SetEnable(true);
}

void SimpleCalcButtons::SetOctal(void)
{
	m_Buttons[0]->SetEnable(true);
	m_Buttons[1]->SetEnable(true);
	m_Buttons[1*m_NHeight+0]->SetEnable(false);
	m_Buttons[1*m_NHeight+1]->SetEnable(true);
	m_Buttons[1*m_NHeight+2]->SetEnable(true);
	m_Buttons[2*m_NHeight+0]->SetEnable(false);
	m_Buttons[2*m_NHeight+1]->SetEnable(true);
	m_Buttons[2*m_NHeight+2]->SetEnable(true);
	m_Buttons[1*m_NHeight+3]->SetEnable(false);

	int i;
	for(i=0;i<6;i++)
		m_Buttons[i*m_NHeight+4]->SetEnable(false);
}

void SimpleCalcButtons::SetDecimal(void)
{
	m_Buttons[0]->SetEnable(true);
	m_Buttons[1]->SetEnable(true);
	m_Buttons[1*m_NHeight+0]->SetEnable(true);
	m_Buttons[1*m_NHeight+1]->SetEnable(true);
	m_Buttons[1*m_NHeight+2]->SetEnable(true);
	m_Buttons[2*m_NHeight+0]->SetEnable(true);
	m_Buttons[2*m_NHeight+1]->SetEnable(true);
	m_Buttons[2*m_NHeight+2]->SetEnable(true);
	m_Buttons[1*m_NHeight+3]->SetEnable(true);

	int i;
	for(i=0;i<6;i++)
		m_Buttons[i*m_NHeight+4]->SetEnable(false);
}

/*void SimpleCalcButtons::Paint(const Rect &update)
{
	Rect cBounds = GetNormalizedBounds();

	SetFgColor(0,0,0,0);
	
	FillRect(cBounds);
}*/

