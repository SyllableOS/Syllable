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

#ifndef CALCBUTTONS_H
#define CALCBUTTONS_H

#include <gui/button.h>
#include <gui/view.h>
#include <util/message.h>

using namespace os;

class SimpleCalcButtons : public View
{
	public:
	SimpleCalcButtons(const Rect &r);
	~SimpleCalcButtons();

	//void Paint(const Rect &);

	void SetBinary(void);
	void SetHexadecimal(void);
	void SetDecimal(void);
	void SetOctal(void);

	private:
	Button	**m_Buttons;

	int		m_NHeight;
	int		m_NWidth;
	int		m_Spacing;
};

enum {
	CB_0 = '0',
	CB_1, CB_2,	CB_3, CB_4, CB_5, CB_6, CB_7, CB_8, CB_9,
	CB_A = 'A',
	CB_B, CB_C, CB_D, CB_E, CB_F,
	CB_EQUAL, CB_PLUS, CB_MINUS, CB_MUL, CB_DIV,
	CB_CLEAR, CB_CLEAR_ALL, CB_CLEAR_MEM, CB_BACKSPACE,
	CB_ADD_MEM, CB_SUB_MEM, CB_READ_MEM,
	CB_LOG, CB_LN, CB_LOG2, CB_SIN, CB_COS, CB_TAN,
	CB_HYP, CB_INV,
	CB_NOT, CB_AND, CB_OR, CB_XOR, CB_INT,
	CB_OPEN_PAR, CB_CLOSE_PAR,
	CB_SIGN, CB_POINT,
	CB_POW2, CB_POW3, CB_POWX, CB_POWE, CB_SQRT
};

#endif /* CALCBUTTONS_H */



