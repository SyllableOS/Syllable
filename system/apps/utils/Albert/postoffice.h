/*
 *  PostOffice - Message handler for AtheOS
 *  Copyright (C) 2001 Henrik Isaksson
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details. 
 *
 *  You should have received a copy of the GNU Library General Public 
 *  License along with this library; if not, write to the Free 
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef POSTOFFICE_H
#define POSTOFFICE_H

#include <util/message.h>
#include <util/looper.h>
#include <util/handler.h>
#include <util/messenger.h>
#include <gui/guidefines.h>
#include <atheos/semaphore.h>

//#define P_O_DEBUG		1

#define PO_BROADCAST_ADDRESS	"%PO%BROADCAST%"
#define PO_ADDRESS_TAG			"%PO%ADDR%"

class Mailbox
{
	public:
	Mailbox *Next() { return m_Next; }
	void Link(Mailbox *next) { m_Next = next; }

	void SetHandler(os::Handler *h) { m_Handler = h; }
	os::Handler *GetHandler() { return m_Handler; }

	void SetName(const char *name);
	const char *GetName() { return m_Name; }

	bool GetUnique() { return m_Unique; }
	void SetUnique(bool u) { m_Unique = u; }

	void PostMessage(os::Message *msg);

	Mailbox();
	~Mailbox();

	private:
	Mailbox		*m_Next;
	os::Handler	*m_Handler;
	os::Messenger	*m_Messenger;
	char			*m_Name;
	bool			m_Unique;
};

class PostOffice : public os::Looper
{
	public:
	static PostOffice *GetInstance();

	bool AddMailbox(const char *Name,
		os::Handler *Handler, bool Unique = false);

	void RemMailbox(const char *Name,
		os::Handler *Handler = NULL);

	void Quit(void);

	protected:

	~PostOffice();

	void HandleMessage(os::Message *Msg);

	bool OkToQuit();

	PostOffice(const std::string &cName);

	bool CheckUnique(const char *name,
			bool unique = false);

	private:

	static PostOffice		*m_Instance;	// The instance pointer.

	Mailbox				*m_Head;		// First mailbox in the list.
//	os::Locker				*m_InUse;		// Remains locked while there
											// are Mailboxes in the list.

	bool					m_Locked;		// Locker problem workaround
};

#ifndef PO_NO_MACROS

#define AddMailbox(n)	PostOffice::GetInstance()->AddMailbox(n, this)

#define RemMailbox(n)	PostOffice::GetInstance()->RemMailbox(n, this)
#define Mail(n,msg)		{ os::Message *__tmpmsg = (msg);\
							  __tmpmsg->AddString(PO_ADDRESS_TAG, n); \
							  if(__tmpmsg->GetCode() == os::M_QUIT) __tmpmsg->SetCode(os::M_LAST_USER_MSG);\
							  PostOffice::GetInstance()->PostMessage(__tmpmsg); }
#define Broadcast(msg)	{ os::Message *__tmpmsg = (msg);\
							  __tmpmsg->AddString(PO_ADDRESS_TAG, PO_BROADCAST_ADDRESS); \
							  if(__tmpmsg->GetCode() == os::M_QUIT) __tmpmsg->SetCode(os::M_LAST_USER_MSG);\
							  PostOffice::GetInstance()->PostMessage(__tmpmsg); }

#endif /* PO_NO_MACROS */

#endif /* POSTOFFICE_H */

