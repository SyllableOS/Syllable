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

#ifndef BASE_N_H
#define BASE_N_H

#include <gui/window.h>
#include <gui/textview.h>
#include <util/message.h>

using namespace os;

class BaseN: public Window {
	public:
	BaseN(const Rect & r);
	~BaseN();
	bool OkToQuit();

	void HandleMessage(Message *);

	void Update(int64 d);

	private:
	TextView	*m_Bin;
	TextView	*m_Hex;
	TextView	*m_Oct;
	TextView	*m_Dec;
	TextView	*m_Base64;
	TextView	*m_Roman;

	bool		m_Visible;
};

void IToBin(int64 val, char *bfr);
int64 BinToI(char *bfr);

#endif


