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

#ifndef STACK_H
#define STACK_H

#ifndef NULL
#define NULL 0
#endif

class StackNode {
	public:
	StackNode() { m_pLink = NULL; }

	void Link(StackNode *p_pLink) { m_pLink = p_pLink; }
	StackNode *Unlink(void) { return m_pLink; }

//	private:
	StackNode *m_pLink;
};

class Stack {
	public:
	Stack();
	~Stack();

	void Push(StackNode *p_pNode)
	{
		p_pNode->Link(m_pTop);
		m_pTop = p_pNode;
	}

	StackNode *Pop(void)
	{
		StackNode *l_pTop = m_pTop;
		if(m_pTop)
			m_pTop = m_pTop->Unlink();
		return l_pTop;
	}

	void Clear(void);

	private:
	StackNode *m_pTop;
};

#endif /* STACK_H */



