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

#include <gui/textview.h>

#include "calc.h"

#include <iostream>

using namespace os;
using namespace std;

class CalcDisplay : public View
{
	public:
	CalcDisplay(const Rect& cFrame);

	virtual void Paint(const Rect &cUpdate);

	virtual void Set(char *p);
	virtual void SetMem(bool);
	virtual void SetInv(bool);
	virtual void SetHyp(bool);
	virtual void SetPar(int);

	~CalcDisplay();

	private:
	CalcWindow* m_pcParent;
	bool	needsFocus;
	char	*m_Contents;
	bool	m_Mem;
	bool	m_Inv;
	bool	m_Hyp;
	int		m_Par;
};



