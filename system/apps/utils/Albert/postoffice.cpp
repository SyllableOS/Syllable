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

#define PO_NO_MACROS 1

#include "postoffice.h"
#include <string.h>
#include <iostream.h>
#include <unistd.h>

Mailbox::Mailbox()
{
	m_Next = 0L;
	m_Handler = 0L;
	m_Name = 0L;
	m_Messenger = 0L;
}

Mailbox::~Mailbox()
{
	if(m_Name) delete []m_Name;
	if(m_Messenger) delete m_Messenger;
}

void Mailbox::SetName(const char *name)
{
	int l = strlen(name);

	if(m_Name) delete []m_Name;

	m_Name = new char[l+1];
	if(m_Name) {
		strcpy(m_Name, name);
	}
}

void Mailbox::PostMessage(os::Message *msg)
{
	if(!m_Messenger) {
		if(m_Handler) {
			m_Messenger = new os::Messenger(m_Handler);
		}
	}

	if(m_Messenger)
		m_Messenger->SendMessage(msg);
}

// ------------------------------------------------------------------ //

PostOffice *PostOffice::m_Instance = 0;

PostOffice *PostOffice::GetInstance()
{
	if(m_Instance == 0) {
		#ifdef P_O_DEBUG
		cout << "PO: Building New PostOffice" << endl;
		#endif
		m_Instance = new PostOffice("ThePostOffice");
		m_Instance->Run();
	}
	return m_Instance;
}

bool PostOffice::OkToQuit()
{
	#ifdef P_O_DEBUG
	cout << "PO: M_QUIT received. Refusing to quit." << endl;
	#endif

	return false;
}

PostOffice::PostOffice(const std::string &cName)
	:Looper(cName)
{
	m_Head = 0L;
//	m_InUse = new os::Locker("PostOfficeInUse", false);
	m_Locked = false;
}

PostOffice::~PostOffice()
{
//	Mailbox *next;

	#ifdef P_O_DEBUG
	cout << "PO: Destructor is running." << endl;
	#endif

	Lock();

	// The destructor should not run if there are any Mailboxes left,
	// so deleting the non-existant Mailboxes won't do us any good, right? =)
/*	while(m_Head) {
		next = m_Head->Next();
		delete m_Head;
		m_Head = next;
	} */

	m_Instance = 0L;

//	delete m_InUse;

	Unlock();
}

void PostOffice::Quit(void)
{
	#ifdef P_O_DEBUG
	//cout << "PO: InUse = " << (m_Locked?"TRUE":"FALSE") << ", " << m_InUse->IsLocked() << endl;
	cout << "PO: Waiting for clients to die..." << endl;
	#endif

	//m_InUse->Lock();
	//cout << "PO: Got a lock." << endl;

	// This part is not optimal... ;) Help appreciated...
	bool l;
	do {
		usleep(50000);	// Wait 50 ms before checking again
		Lock();
		l = m_Locked;	
		Unlock();
	} while(l);

	
	// If some one adds Mailboxes here, we've got a deadlock...

	//#ifdef P_O_DEBUG
	//cout << "PO: InUse = " << (m_Locked?"TRUE":"FALSE") << ", " << m_InUse->IsLocked() << endl;
	//#endif

	//m_InUse->Unlock();
	// If some one adds Mailboxes here, we've got a crash...

	// Conclusion: No Mailboxes may be added after calling Quit.

	#ifdef P_O_DEBUG
	cout << "PO: Signing off..." << endl;
	#endif

	Looper::Quit();

	// If somebody adds Mailboxes here, a new instance will be created...
}

bool PostOffice::AddMailbox(const char *name,
	os::Handler *handler, bool unique)
{
	bool		ret = true;
	Mailbox	*mb;

	Lock();

	if(!CheckUnique(name, unique))
		ret = false;

	if(ret) {
		mb = new Mailbox;
		if(mb) {
			mb->SetName(name);
			mb->SetHandler(handler);
			mb->SetUnique(unique);
			mb->Link(m_Head);
			m_Head = mb;

			if(!m_Locked) {
				m_Locked = true;
			//	m_InUse->Lock();

				#ifdef P_O_DEBUG
				cout << "PO: PostOffice is in use." << endl;
				#endif
			}

			#ifdef P_O_DEBUG
			cout << "PO: Added Mailbox: " << name << endl;
			#endif

			ret = false;
		}
	}

	Unlock();

	return ret;
}

void PostOffice::RemMailbox(const char *name, os::Handler *handler)
{
	Mailbox *next, *current, *prev = 0L;

	Lock();

	current = m_Head;

	while(current) {
		next = current->Next();

		if( (strcmp(current->GetName(), name) == 0) &&
		    (!handler || handler == current->GetHandler()) ) {

			if(prev)
				prev->Link(current->Next());
			else
				m_Head = current->Next();
			delete current;

			break;
		} else {
			prev = current;
		}

		current = next;
	}

	if(!m_Head) {
		m_Locked = false;
	//	m_InUse->Unlock();

		#ifdef P_O_DEBUG
		cout << "PO: PostOffice no longer in use." << endl;
		#endif
	}

	Unlock();

	#ifdef P_O_DEBUG
	cout << "PO: Deleted Mailbox: " << name << endl;
	#endif
}

void PostOffice::HandleMessage(os::Message *msg)
{
	const char *dest = 0L;

	#ifdef P_O_DEBUG
	int		recipents_found = 0;
	#endif


	msg->FindString(PO_ADDRESS_TAG, &dest);

	if(dest) {
		Mailbox *current;

		#ifdef P_O_DEBUG
		cout << "PO: Message to: " << dest << endl;
		#endif

		// It would be better to intercept messages before OkToQuit gets
		// a chance to snatch them. Don't know if that is possible, so
		// in the meantime:
		if(msg->GetCode() == os::M_LAST_USER_MSG)
			msg->SetCode(os::M_QUIT);

		Lock();

		current = m_Head;

		if(strcmp(dest, PO_BROADCAST_ADDRESS) == 0) {
			while(current) {
				current->PostMessage(msg);

				#ifdef P_O_DEBUG
				recipents_found++;
				#endif

				current = current->Next();
			}
		} else {
			while(current) {
				if(strcmp(current->GetName(), dest) == 0) {
					current->PostMessage(msg);

					#ifdef P_O_DEBUG
					recipents_found++;
					#endif
				}
				current = current->Next();
			}
		}

		Unlock();

		#ifdef P_O_DEBUG
		if(!recipents_found)
			cout << "PO: Recipent not found, message lost: " << dest << endl;
		#endif

		return;
	}

	#ifdef P_O_DEBUG
	else
		cout << "PO: Message without address tag received." << endl;
	#endif

	// Apparently not ment for us, pass it on.
	Looper::HandleMessage(msg);
}

bool PostOffice::CheckUnique(const char *name, bool unique)
{
	Mailbox *next = m_Head;

	while(next) {
		if(strcmp(next->GetName(), name) == 0)
			return !(unique || next->GetUnique());
		next = next->Next();
	}

	return true;
}
