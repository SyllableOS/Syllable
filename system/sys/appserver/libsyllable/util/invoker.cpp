
/*  libgui.so - the GUI API/appserver interface for AtheOS
 *  Copyright (C) 1999  Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
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

#include <util/invoker.h>
#include <util/message.h>
#include <util/application.h>

using namespace os;

/** Default constructor.
 * \par Description:
 *	The default constructor initialte the invoker to target the application
 *	looper but with no message. An message must be assigned with SetMessage()
 *	before the invoker can be used.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Invoker::Invoker():m_cMessenger( Application::GetInstance(  ) )
{
	m_pcMessage = NULL;
	m_pcReplyTo = NULL;
}

/** Constructor
 * \par Description:
 *	Construct a invoker with the specified message that targets the
 *	application looper.
 * \note
 *	The invoker will take ownership over the message and will delete
 *	it when another message is assigned or the messenger is deleted.
 *	The message must therefor be allocated on the heap with the "new"
 *	operator and must not be deleted by the application.
 * \param pcMsg
 *	The message to be sendt when invoked. The invoker takes ownership
 *	over the message.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Invoker::Invoker( Message * pcMsg ):m_cMessenger( Application::GetInstance() )
{
	m_pcMessage = pcMsg;
	m_pcReplyTo = NULL;
}

/** Constructor
 * \par Description:
 *	Construct a invoker with a message and a target.
 *	When invoked the \p pcMsg will be sendt to the target specified
 *	by \p pcHandler and/or \p pcLooper.
 * \par
 *	Look at os::Messenger for a more detailed description on how the
 *	to target a looper or handler. The invoker will pass these parameters
 *	to an internal os::Messenger that is later used to send messages
 *	so the targeting rules of os::Messenger goes for the os::Invoker
 *	aswell.
 * \note
 *	The invoker will take ownership over the message and will delete
 *	it when another message is assigned or the messenger is deleted.
 *	The message must therefor be allocated on the heap with the "new"
 *	operator and must not be deleted by the application.
 * \param pcMsg
 *	The message to be sendt when invoked. The invoker takes ownership
 *	over the message.
 * \param pcHandler
 *	The handler that should receive messages sendt by this invoker
 *	or NULL if a looper is to be destined.
 * \param pcLooper
 *	If \p pcHandler is NULL messages will be sendt to this looper
 *	without a specific target.
 * \sa os::Messenger
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Invoker::Invoker( Message * pcMsg, const Handler * pcHandler, const Looper * pcLooper )
{
	m_pcMessage = pcMsg;
	m_cMessenger = Messenger( pcHandler, pcLooper );
	m_pcReplyTo = NULL;
}

/** Constructor
 * \par Description:
 *	Construct a invoker with a message and a target.
 *	When invoked the \p pcMsg will be sendt to the target specified
 *	by \p cTarget
 * \note
 *	The invoker will take ownership over the message and will delete
 *	it when another message is assigned or the messenger is deleted.
 *	The message must therefor be allocated on the heap with the "new"
 *	operator and must not be deleted by the application.
 * \param pcMsg
 *	The message to be sendt when invoked. The invoker takes ownership
 *	over the message.
 * \param cTarget
 *	A messenger specifying this invokers target
 * \sa os::Messenger
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/


Invoker::Invoker( Message * pcMsg, const Messenger & cTarget )
{
	m_pcMessage = pcMsg;
	m_cMessenger = cTarget;
	m_pcReplyTo = NULL;
}

/** Destructor
 * \par Description:
 *	Free any resources allocated by the invoker and delete the user
 *	supplied message object. Since the message object is deleted here
 *	it is of course very important that you don't keep any external
 *	references to it after the invoker has been deleted. Neither should
 *	you ever delete a message that have been given to an invoker.
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Invoker::~Invoker()
{
	delete m_pcMessage;
}

/** Assign a new message to the invoker
 * \par Description:
 *	Assign a new message to the invoker. You can call SetMessage() to
 *	assign a message to a invoker that didn't get one in the constructor
 *	or to replace the old message. If the invoker already have a message
 *	assigned it will be deleted first.
 *
 *	If the message is set to NULL the invoker will be disabled until
 *	a new message is assigned.
 * \note
 *	The invoker will take ownership over the message and will delete
 *	it when another message is assigned or the messenger is deleted.
 *	The message must therefor be allocated on the heap with the "new"
 *	operator and must not be deleted by the application.
 * \param pcMsg
 *	Pointer to the new message to be sendt when invoked. The invoker
 *	takes ownership over the message.
 * \return Always 0.
 * \sa GetMessage(), MessageChanged(), Invoke()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Invoker::SetMessage( Message * pcMsg )
{
	Message *pcOld = m_pcMessage;

	m_pcMessage = pcMsg;
	if( pcOld != NULL )
	{
		MessageChanged( pcOld );
		delete pcOld;
	}
	else
	{
		MessageChanged( Message() );
	}
	return ( 0 );
}

/** Get the message currently assigned to the invoker.
 * \par Description:
 *	Get the message currently assigned to the invoker through the
 *	constructor or the SetMessage() member.
 * \note
 *	The message will be deleted if SetMessage() is called or if the
 *	messenger is deleted so make sure to not reference the message
 *	after any such event.
 * \return Pointer to the currently assigned message or NULL if no message
 *	has been assigned.
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Message *Invoker::GetMessage() const
{
	return ( m_pcMessage );
}

/** Get the code field from the currently assigned message.
 * \par Description:
 *	Short for GetMessage()->GetCode(). If not message is assigned 0
 *	will be returned.
 * \note
 *	Since 0 is a valid value for a message-code it might be necessarry to
 *	check if the message exists with GetMessage() before calling this member.
 * \return The message-code of the currently assigned message or 0 if not
 *	message is currently assigned.
 * \sa GetMessage(), SetMessage()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

uint32 Invoker::GetCode( void ) const
{
	if( NULL != m_pcMessage )
	{
		return ( m_pcMessage->GetCode() );
	}
	else
	{
		return ( 0 );
	}
}

/** Set a new message target.
 * \par Description:
 *	Set a new message target for the invoker. Ths \p pcHandler and \p pcLooper
 *	parameters is passed directly to the internal os::Messenger used to
 *	send messages from this invoker. Look at the os::Messenger documentation
 *	for a more detailed description on how to target messages.
 * \param pcHandler
 *	The handler that should receive messages sendt by this invoker
 *	or NULL if a looper is to be destined.
 * \param pcLooper
 *	If \p pcHandler is NULL messages will be sendt to this looper
 *	without a specific target.
 * \return 0 if the a valid target was set. -1 if no valid target was
 *	speciefied.
 * \sa SetMessage(), TargetChanged(), Invoke()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Invoker::SetTarget( const Handler * pcHandler, const Looper * pcLooper )
{
	Messenger cOldTarget = m_cMessenger;

	m_cMessenger = Messenger( pcHandler, pcLooper );
	TargetChanged( cOldTarget );
	return ( ( m_cMessenger.IsValid()? 0 : -1 ) );
}

/** Set a new message target.
 * \par Description:
 *	Set a new message target.
 * \param cMessenger
 *	The messenger that should be used to target messages from this
 *	invoker.
 * \return 0 if the a valid target was set. -1 if no valid target was
 *	speciefied.
 * \sa SetMessage(), TargetChanged(), Invoke()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Invoker::SetTarget( const Messenger & cMessenger )
{
	Messenger cOldTarget = m_cMessenger;

	m_cMessenger = cMessenger;
	TargetChanged( cOldTarget );
	return ( ( m_cMessenger.IsValid()? 0 : -1 ) );
}

/** Check if the target lives in our process.
 * \par Description:
 *	Check if the target lives in our process.
 * \return true if the target is local or false if the target lives in a
 *	remote process.
 * \sa SetTarget(), GetTarget(), os::Messenger
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool Invoker::IsTargetLocal() const
{
	return ( m_cMessenger.IsTargetLocal() );
}

/** Get the current target.
 * \par Description:
 *	Get the current looper/handler targeted by this invoker.
 *	Look at os::Messenger::GetTarget() for a more detailed description.
 * \param ppcLooper
 *	If non-NULL a pointer to the looper owning the target will be written
 *	here.
 * \return Pointer to the destined os::Handler (this might be a looper if
 *	no specific handler is targeted) or NULL if the messenger is invalid
 *	or targets a handled/looper in a remote process.
 * \sa SetTarget(), os::Messenger::GetTarget(), os::Messenger
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Handler *Invoker::GetTarget( Looper ** ppcLooper ) const
{
	return ( m_cMessenger.GetTarget( ppcLooper ) );
}

/** Get the messenger used to target messages sendt by the invoker.
 * \par Description:
 *	Get the messenger used to target messages sendt by the invoker.
 * \return A copy of the internal messenger used to target messages from
 *	the invoker.
 * \sa SetTarget(), Invoke()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Messenger Invoker::GetMessenger( void ) const
{
	return ( m_cMessenger );
}

/** Set a reply target for messages sendt by this messenger.
 * \par Description:
 *	Specify a target for replies sendt with os::Message::SendReply()
 *	on messages sendt by this invoker.
 * \par
 *	Look at the os::Messenger class for more info on syncronous/asyncronous
 *	replies and messaging in general.
 * \param pcHandler
 *	The handler that should receive replies on messages sendt by the
 *	invoker or NULL to disable replies.
 * \return Always 0.
 * \sa GetHandlerForReply(), os::Messenger, os::Message, os::Handler
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Invoker::SetHandlerForReply( Handler * pcHandler )
{
	m_pcReplyTo = pcHandler;
	return ( 0 );
}

/** Get the current reply target for replies on messages sendt by the invoker.
 * \par Description:
 *	Get the current reply target for replies on messages sendt by the invoker.
 *	This is the handler assigned with SetHandlerForReply().
 * \return Pointer to the current reply target.
 * \par Error codes:
 * \sa SetHandlerForReply()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

Handler *Invoker::GetHandlerForReply() const
{
	return ( m_pcReplyTo );
}

/** Send the current message to the current target.
 * \par Description:
 *	Call Invoke() to pass the currently assigned message (or optionally
 *	an "external" message passed to Invoke()) to the current target.
 * \par
 *	This is the member that is called by GUI controls and such to
 *	notify it's target about a change.
 * \par
 *	The message to be passed (whether it is supplied through the \p pcMsg
 *	parameter or the internal message) will be copyed and a pointer to the
 *	invoker is added under the name "source". Then the virtual member
 *	Invoked() is called to allow classes that overload os::Invoker to
 *	add date to the message or even cancel the operation before it is
 *	sendt to the current target.
 * \note
 *	The receiver of the message can find a pointer to the invoker
 *	that sendt the message by calling FindPointer( "source", &pPtr )
 *	on the incoming message.
 * \par
 *	Since os::Invoker often are mixed into other classe with
 *	multiple inheritance greate care must be taken when casting
 *	the void pointer from os::Message::FindPointer() to the real
 *	object. The most common usage of invoker is through the
 *	os::Control class which inherits from both os::View and
 *	os::Invoker. If you receive a message from for example
 *	a os::Button (which again inherits from os::Control) you
 *	can not cast the pointer directly to os::Button but you
 *	must let the compiler know that the pointer is really
 *	an os::Invoker pointer first.
 * \par
 *	The following example will put a pointer to the os::Button that
 *	sendt the message into \b pcButton if the message was sendt from
 *	a os::Button. If the source was not an os::Button the dynamic_cast
 *	will fail and \b pcButton will be NULL.
 *     	\code
 *		void* pSource = NULL;
 *		pcMsg->FindPointer( "source", &pSource );
 *		os::Button* pcButton = dynamic_cast<os::Button*>(static_cast<os::Invoker*>(pSource));
 *	\endcode
 * \note
 *	If no message is currently assigned and \p pcMsg is NULL, or if
 *	no valid target has been specifed no message will be sendt.
 * \param pcMsg
 *	An optional message to be sendt instead of the internal message.
 *	The message will be copyed and you retain ownership over it.
 *	If this parameter is NULL the internal message will be used.
 *	It is often better to overload Invoked() and do per-message
 *	modifications there than to supply "custom" messages.
 * \return On success 0 is returned. On failure -1 is returned and a error
 *	code is written to the global variable \b errno. A message that is
 *	canceled by Invoked() are not considered a failure.
 * \sa Invoked(), SetTarget(), SetMessage(), os::Invoker, os::Message
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

status_t Invoker::Invoke( Message * pcMsg )
{
	if( NULL != pcMsg )
	{
		Message cMsg( *pcMsg );
		cMsg.AddPointer( "source", static_cast < void *>( this ) );

		if( Invoked( &cMsg ) )
		{
			return ( m_cMessenger.SendMessage( &cMsg, m_pcReplyTo ) );
		}
		else
		{
			return ( 0 );
		}
	}
	else
	{
		if( m_pcMessage != NULL )
		{
			Message cMsg( *m_pcMessage );
			cMsg.AddPointer( "source", static_cast < void *>( this ) );

			if( Invoked( &cMsg ) )
			{
				return ( m_cMessenger.SendMessage( &cMsg, m_pcReplyTo ) );
			}
			else
			{
				return ( 0 );
			}
		}
	}
	return ( -1 );
}

/** Intercept outgoing messages.
 * \par Description:
 *	This member is called from Invoke() just before a message is sendt
 *	to the target.
 * \par
 *	This allow classes that inherits from os::Invoker to add data to or
 *	otherwhice modify the message before it is sendt. The message can
 *	also be canceled entirely by returning \b false from this member.
 * \par
 *	The message passed to Invoked() is a copy of the internal message
 *	or the message passed to Invoke() (if any) so any changes made here
 *	will not affect the internal message or the message passed to Invoke().
 *	When this method returns the message will imidiatly be sendt to the
 *	target and then discarded (unless \b false is returned in which case
 *	the message is simply discarded).
 * \par
 *	The default implementation of this member does nothing and return
 *	\b true.
 * \param pcMessage
 *	Pointer to the message that is about to be sendt. You can do any
 *	modification you like to this message (but never delete it).
 * \return Normally you should return \b true to indicate that the message
 *	should be sendt. You can however return \b false if you for some reason
 *	want to cancel the invokation.
 * \sa Invoke(), SetMessage(), SetTarget()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

bool Invoker::Invoked( Message * pcMessage )
{
	return ( true );
}

/** Virtual hook called by the system after the target has been changed.
 * \par Description:
 *	You can overload this member if you need to be notified whenever
 *	the target is changed by one of the SetTarget() members.
 * \par
 *	This membert is called after the new target have been assigned
 *	but the old target is available through the \p cOldTarget argument.
 * \param cOldTarget
 *	The previous target for this invoker.
 * \sa SetTarget(), MessageChanged()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Invoker::TargetChanged( const Messenger & cOldTarget )
{
}

/** Virtual hook called by the system after the message has been changed.
 * \par Description:
 *	You can overload this member if you need to be notified whenever
 *	the internal message is changed by the SetMessage() member.
 * \par
 *	This membert is called after the new message have been assigned
 *	but the old message is available through the \p cOldMsg argument.
 * \param cOldMsg
 *	The previous message assigned to this invoker.
 * \sa SetMessage(), TargetChanged()
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

void Invoker::MessageChanged( const Message & cOldMsg )
{
}
