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

#include "remsa.h"
#include "postoffice.h"
#include "resources/Calculator.h"

enum {
	ID_COPY = 5, ID_CLEAR, ID_SAVE
};

PaperRoll::PaperRoll(const Rect& cWindowBounds, char* pzTitle, char* pzBuffer, uint32 nResizeMask , uint32 nFlags)
	:TextView (cWindowBounds, pzTitle, pzBuffer, nResizeMask , nFlags)
{
	m_ContextMenu=new Menu(Rect(0,0,0,0),"",ITEMS_IN_COLUMN);
	m_ContextMenu->AddItem(MSG_PAPERROLL_CMENU_COPY, new Message(ID_COPY));
	m_ContextMenu->AddItem(MSG_PAPERROLL_CMENU_CLEAR, new Message(ID_CLEAR));
	//m_ContextMenu->AddItem(MSG_PAPERROLL_CMENU_SAVE, new Message(ID_SAVE));
}

void Remsa::HandleMessage(Message *msg)
{
	const char *str=0L;

	switch(msg->GetCode()) {
		case 1:
			msg->FindString("text", &str);
			_AddText( str );
			break;
		case ID_COPY:
			m_TextView->SelectAll();
			m_TextView->Copy();
			m_TextView->ClearSelection();
			break;
		case ID_CLEAR:
			m_TextView->Clear();
			break;
		case ID_SAVE:
			break;
		default:
			Window::HandleMessage(msg);
	}
}

void Remsa::Show(void)
{
	m_Visible = !m_Visible;
	Window::Show(m_Visible);
}

Remsa::Remsa(const Rect & r)
	:Window(r, "PaperRoll", MSG_PAPERROLL_TITLE, 0, CURRENT_DESKTOP)
{
	Rect bounds = GetBounds();

	m_Visible = false;

	m_TextView = new PaperRoll(bounds, "tv", "", CF_FOLLOW_ALL, WID_FULL_UPDATE_ON_RESIZE | WID_WILL_DRAW);
	
	m_TextView->SetMultiLine(true);
	m_TextView->SetReadOnly(true);

	Font* pcAppFont = new Font ( DEFAULT_FONT_FIXED );
	pcAppFont->SetSize( 8 );

	m_TextView->SetFont(pcAppFont);
	pcAppFont->Release();

	AddChild(m_TextView);

	AddMailbox("PaperRoll");
}

Remsa::~Remsa()
{
	RemMailbox("PaperRoll");
}

bool Remsa::OkToQuit(void)
{
	return true;
}

void Remsa::_AddText( const char *txt )
{
	if( txt ) {
		m_TextView->Insert(txt, false);
	}
}

