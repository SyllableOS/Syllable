/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2006 Syllable Team
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

#ifndef	__F_UTIL_EVENT_H__
#define	__F_UTIL_EVENT_H__

#include <gui/guidefines.h>
#include <util/message.h>
#include <util/looper.h>
#include <vector>

namespace os
{
#if 0
}
#endif // Fool Emacs auto-indent



/** Events.
 * \ingroup util
 * \par Description:
 * The event class provides methods for interprocess communication.
 *
 * \par Registering and handling events:
 *
 * All events are identified using an id string. Currently this can be any string but in future
 * revisions it has to look like this:
 * <class>/<subclass>/<name>
 * \par
 * In this example we want to create an event named "app/TestApp/GetTestString".
 * \par
 * A good place to register the event is in the constructor of your application window.
 * Add a "os::Event* m_pcTestEvent" member to the class and add this code in the constructor:
 * \code
 * m_pcTestEvent = new os::Event();
 * m_pcTestEvent->Register( "app/TestApp/GetTestString", "This is a test event" (description),
 * 			this (handler), MSG_GET_TEST_STRING );
 * \endcode
 * That's it. If another application uses this event your window will receive a message with the
 * code MSG_GET_TEST_STRING which can include additional data supplied by the caller.
 * \par
 * In this example we want to send a reply. So we put this code into our HandleMessage() method:
 * \code
 * case MSG_GET_TEST_STRING:
 * {
 *   if( !pcMessage->IsSourceWaiting() )
 *     break;
 *   int32 nCode;
 *   pcMessage->FindInt32( "reply_code", &nCode );
 *   os::Message cReply( nCode );
 *   cReply.AddString( "test_string", "This is a test" );
 *   pcMessage->SendReply( &cReply );
 *   break;
 * }
 * \endcode
 * As you can see we first check if the caller is waiting for a reply. If it is we create the reply
 * message and set the code to the value supplied by the caller (the field is added by the os::Event
 * class, not by the calling application itself). Please note that if you do not set the reply code,
 * things will break.
 * \par
 * If you follow the kernel log you will see that Syllable complains when you close your application
 * because you haven´t deleted the event. Put this code into your destructor:
 * \code
 * delete( m_pcTestEvent );
 * \endcode
 *
 * \par Calling an event:
 *
 * We now want to call this event from another application. This code will do this:
 * \code
 * os::Event cEvent;
 * if( cEvent.SetToRemote( "app/TestApp/GetTestString", 0 (index) ) == 0 )
 * {
 *   os::Message cMsg;
 *   cEvent.PostEvent( &cMsg, this (reply handler), MSG_REPLY (reply message code) );
 * }
 * \endcode
 * This code will send an empty message to the event and the reply is sent to the reply handler 
 * where we can extract the reply string:
 * \code
 * case MSG_REPLY:
 * {
 *   os::String zTestString;
 *   if( pcMessage->FindString( "test_string", &zTestString ) != 0 )
 * 	break;
 *   ...
 *   break;
 * }
 * \endcode
 *
 * \par Event monitors:
 *
 * If you want to know what events with a known id string are available, you could frequently 
 * use SetToRemote() to poll for the events. This requires additional code and reduces performance.
 * To solve this problem you can set a monitor to events with a given id string. Some code:
 * \par
 * \code
 * m_pcMonitorEvent = new os::Event();
 * m_pcMonitorEvent->SetToRemote( "app/TestApp/GetTestString", -1 );
 * m_pcMonitorEvent->SetMonitorEnabled( true, this (handler), MSG_EVENT_CHANGED (code) );
 * \endcode
 * \par
 * You might wonder what the index -1 means. If you use this index then the os::Event class 
 * will not try to get any information about the event because you cannot know if an event with
 * this id string already exists. Make sure you delete the monitor before your application quits!
 * The message sent to the handler will contain a boolean field "event_registered" or
 * "event_unregistered". It can also happen that both fields are not present. The next chapter
 * deals with this.
 * 
 * \par Broadcasting messages:
 * 
 * The event monitors can also be used to broadcast messages. If you call the PostEvent() method
 * of an event object that you have registered yourself then the message is sent to all monitors.
 * Please note that replies are not supported in this case.
 * 
 * \par Data storage:
 * 
 * Sometimes a direct communication is not really necessary if you just want to access some 
 * information supplied by another application. In this case this application can broadcast
 * the message like in chapter 4. The application that wants to get the information doesn´t have
 * to set a monitor to this event. They can use the GetLastEventMessage() method to get the last 
 * message which has been sent.
 * \author	Arno Klenke
 *****************************************************************************/


class Event
{
public:
	Event();
	~Event();
	
	static Event*	Register( String zID, String zDescription, Handler* pcTarget, int nMessageCode, uint32 nFlags = 0 );
	status_t		SetToRemote( String zID, int nIndex = 0 );
	void			Unset();
	bool			IsRemote();
	status_t		GetRemoteInfo( proc_id* pnProcess, port_id* phPort, int* pnMessageCode, os::String* pzDesc );
	status_t		GetRemoteChildren( std::vector<os::String>* pacList );
	status_t		SetMonitorEnabled( bool bEnabled, Handler* pcTarget = NULL, int nMessageCode = -1 );
	bool			IsMonitorEnabled();
	status_t		PostEvent( Message* pcData, Handler* pcReplyHandler = NULL, int nReplyCode = M_REPLY );
	status_t		GetLastEventMessage( os::Message* cReply );
private:
	class Private;
	Private* m;
};


}

#endif	// __F_UTIL_EVENT_H__










