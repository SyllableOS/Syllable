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


#include "stack.h"

Stack::Stack()
{
	m_pTop = NULL;
}

Stack::~Stack()
{
	Clear();
}

void Stack::Clear(void)
{
	StackNode *l_pDel;

	do {
		l_pDel = m_pTop;
		if(l_pDel) {
			m_pTop = m_pTop->Unlink();
			delete l_pDel;
		}
	} while(l_pDel);

	m_pTop = NULL;
}
