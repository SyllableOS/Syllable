/*  libatheos.so - the highlevel API library for Syllable
 *  Copyright (C) 2001 Kurt Skauen
 *  Copyright (C) 2003 The Syllable Team
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

#include <util/messagefilter.h>
#include <util/message.h>

using namespace os;

class MessageFilter::Private {
	public:

    Handler* 				  m_pcTarget;
	int32   				  m_nFilterCode;

	Private() {
		m_nFilterCode = 0;
		m_pcTarget = NULL;
	}

};

MessageFilter::MessageFilter()
{
	m_pcHandler = NULL;
	m = new Private;
}

/** Constructor
 * \par Description:
 *	Creates a message filter that reroutes messages with a certain code to
 *	a specific Handler object.
 * \param nFilterCode The filter code to respond to.
 * \param pcTarget New target for messages.
 * \sa Looper::AddCommonFilter()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
MessageFilter::MessageFilter( int32 nFilterCode, Handler* pcTarget )
{
	m_pcHandler = NULL;
	m = new Private;
	m->m_pcTarget = pcTarget;
	m->m_nFilterCode = nFilterCode;
}

MessageFilter::~MessageFilter()
{
	delete m;
}

/** Message filtering function.
 * \par Description:
 *	This method can be overridden in subclasses to attain the desired filtering.
 *	By default it diverts messages with a certain code to a different target,
 *	set with SetTarget().
 * \param pcMessage The message to filter.
 * \param ppcHandler The handler to send to. Can be changed in this function.
 * \returns MF_DISPATCH_MESSAGE or MF_DISCARD_MESSAGE.
 * \sa SetTarget(), SetFilterCode().
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
MessageFilterResult MessageFilter::Filter( Message* pcMessage, Handler** ppcHandler )
{
	if( m->m_nFilterCode == pcMessage->GetCode() ) {
		*ppcHandler = m->m_pcTarget;
	}
	return MF_DISPATCH_MESSAGE;
}

/** Get filter code.
 * \par Description:
 * \sa SetFilterCode()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
int32 MessageFilter::GetFilterCode() const
{
	return ( m->m_nFilterCode );
}

/** Set filter code.
 * \par Description:
 *	Messages with a Code field that matches the number specified with this
 *	method will be diverted by the Filter() function.
 * \param nCode What code to react to.
 * \sa GetFilterCode(), Message::GetCode(), Filter()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void MessageFilter::SetFilterCode( int32 nCode )
{
	m->m_nFilterCode = nCode;
}

/** Set target for diverted messages.
 * \par Description:
 *	Messages matched by Filter() will be diverted to the Handler you specify
 *	with this method.
 * \param pcTarget Where to send filtered messages.
 * \sa GetTarget()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
void MessageFilter::SetTarget( Handler* pcTarget )
{
	m->m_pcTarget = pcTarget;
}

/** Get target for diverted messages.
 * \par Description:
 * \sa SetTarget()
 * \author Henrik Isaksson (henrik@isaksson.tk)
 *****************************************************************************/
Handler* MessageFilter::GetTarget() const
{
	return m->m_pcTarget;
}

