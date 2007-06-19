
/*  libsyllable.so - the GUI API/appserver interface for AtheOS
 *	Copyright (C) 2007	Rick Caudill
 *  Copyright (C) 1999 - 2000 Kurt Skauen
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

#include <atheos/kernel.h>
#include <atheos/msgport.h>

#include <appserver/protocol.h>


#include <util/clipboard.h>
#include <util/message.h>
#include <util/locker.h>
#include <util/string.h>
#include <util/application.h>

using namespace os;

/**internal*/
class Clipboard::Private
{
public:
	Private(const os::String& name) : m_cMutex("clipboard_lock")
	{
		
		
		if (name == "")
			m_cName			=	os::String("__system_clipboard__");
		else
			m_cName			=	name;
			
		m_cBuffer		=	Message(0);
		m_hReplyPort	=	create_port("app_reply",DEFAULT_PORT_SIZE);
		m_hServerPort	=	get_app_server_port();
		m_bCleared		=	false;
		
	}
	
public:
	Locker			m_cMutex;
    String			m_cName;
    port_id			m_hServerPort;
    port_id			m_hReplyPort;
    Message			m_cBuffer;
    bool			m_bCleared;
};


/** Constructor.
 * \par Description:
 * os::Clipboard constructor. 
 * \param cName - The name of the clipboard.  If you pass a null string or just 
 *				  call the contructor the name that will be __system_clipboard__
 *
 * \author	Rick Caudill(rick@syllable.org)
 *****************************************************************************/
Clipboard::Clipboard( const String& cName)
{
	m = new Private(cName);
}


/** Destructor
 * \par Description:
 * os::Clipboard destructor will delete the reply port, the event, and then get rid of all the
 * private information. 
 * \author	Rick Caudill(rick@syllable.org)
 *****************************************************************************/
Clipboard::~Clipboard()
{
	delete_port( m->m_hReplyPort );
	delete m;
}


/** Locks the clipboard
 * \par Description:
 * This function will always return true, but before doing so it sets the mutex
 * of this clipboard to lock and then makes note that we haven't cleared the clipboard.
 *
 * \note
 * Before doing anything with the clipboard, you should lock it.
 *
 * \note
 * If you Lock something then you must Unlock it.
 *
 *	\sa
 *		Unlock(), os::Locker
 * \author	Rick Caudill(rick@syllable.org)
 *****************************************************************************/
bool Clipboard::Lock()
{
	m->m_cMutex.Lock();
	m->m_bCleared = false;
	return ( true );
}


/** Unlocks the clipboard
 * \par Description:
 * This function unlocks the clipboard, but only after clearing the old clips out of
 * the clipboard.
 *
 * \note
 * Before doing anything with the clipboard, you should Lock it.
 *
 * \note
 * If you Lock something then you must Unlock it.
 *
 *	\sa
 *		Lock(), os::Locker
 * \author	Rick Caudill(rick@syllable.org)
 *****************************************************************************/
void Clipboard::Unlock()
{
	m->m_cBuffer.MakeEmpty();
	m->m_cMutex.Unlock();
}


/** Clears the clipboard
 * \par Description:
 * This function doesn't actually clear the clipboard, but it sets a flag so when the
 * programmer calls GetData() the data is cleared.
 *
 * \note
 * You cannot use the Clipboard class outside of an application object, because the event is sent to the current application object.
 *
 * \note
 * Before doing anything with the clipboard, you should Lock it.
 *
 * \note
 * If you Lock something then you must Unlock it.
 *
 *	\sa
 *		Unlock(), os::Locker
 * \author	Rick Caudill(rick@syllable.org)
 *****************************************************************************/
void Clipboard::Clear()
{
	m->m_bCleared = true;
}


/** Returns the data that was added to the clipboard.
 * \par Description:
 * This function gets the data that was added to the clipboard last and it then returns it
 * 
 * \note
 * This function calls the syllable appserver itself.  
 *
 * \note
 * You cannot use the Clipboard class outside of an application object, because the event is sent to the current application object.
 *
 * \note
 * Before doing anything with the clipboard, you should lock it.
 *
 * \note
 * If you Lock something then you must Unlock it.
 *
 *	\sa
 *		Unlock(), Commit(), os::Locker
 * \author	Rick Caudill(rick@syllable.org)
 *****************************************************************************/
Message *Clipboard::GetData()
{
	//if we are clear then
	if( m->m_bCleared == true )
	{
		//empty out the contents os the message
		//and then set cleared to false
		m->m_cBuffer.MakeEmpty();
		m->m_bCleared = false;
	}
	
	
	else
	{
		
		//these are defined in include/appserver
		DR_GetClipboardData_s sReq( m->m_cName.c_str(), m->m_hReplyPort );
		DR_GetClipboardDataReply_s sReply;


		//send to the appserver that we want to get the clipboard data, if we don't get the data then we flag an error and exit
		if( send_msg( m->m_hServerPort, DR_GET_CLIPBOARD_DATA, &sReq, sizeof( sReq ) ) != 0 )
		{
			dbprintf( "Error: Clipboard::GetData() failed to send request to server: %s\n", strerror( errno ) );
			goto error;
		}
		
		//get a reply from the appserver
		if( get_msg( m->m_hReplyPort, NULL, &sReply, sizeof( sReply ) ) < 0 )
		{
			dbprintf( "Error: Clipboard::GetData() failed to read reply from server: %s\n", strerror( errno ) );
			goto error;
		}
		
		//if the size of the message isn't 0
		if( sReply.m_nTotalSize > 0 )
		{
			//if we can unflatten the reply buffer then we can return it
			//else flag an error stating that we cannot unpack the message
			//empty out the buffer and head to the error
			if( m->m_cBuffer.Unflatten( sReply.m_anBuffer ) != 0 )
			{
				dbprintf( "Error: Clipboard::GetData() failed to unpack the message\n" );
				m->m_cBuffer.MakeEmpty();
				goto error;
			}
		}
		else
		{
			m->m_cBuffer.MakeEmpty();
			m->m_bCleared = false;
		}
	}
	return ( &m->m_cBuffer );
	
	error:
		return ( NULL );
}

/** Adds the data to the clipboard
 * \par Description:
 * This function will add the data clipboard last.  This is just a container for a Clip that is sent to the appserver and then
 * is retrieved via GeData().
 * 
 * \note
 * This function sends to the syllable appserver itself(via send_msg).  
 *
 * \note
 * You cannot use the Clipboard class outside of an application object, because the event is sent to the current application object.
 *
 * \note
 * Before doing anything with the clipboard, you should lock it.
 *
 * \note
 * If you Lock something then you must Unlock it.
 *
 *	\sa
 *		Unlock(), GetData(), os::Locker
 * \author	Rick Caudill(rick@syllable.org)
 *****************************************************************************/
void Clipboard::Commit()
{
	//defined in include/appserver
	DR_SetClipboardData_s sReq;
	
	//get the size of the buffer
	int nSize = m->m_cBuffer.GetFlattenedSize();

	//copy the name of the the clipboard to the name of the reply
	strcpy( sReq.m_zName, m->m_cName.c_str() );
	
	//set the reply port
	sReq.m_hReply = m->m_hReplyPort;	// Just used as an source ID so the server wont interleave multiple commits
	
	//the total size
	sReq.m_nTotalSize = nSize;

	//if the size <= the frament size(defined in include/appserver/protocol.h as 1024*32)
	if( nSize <= CLIPBOARD_FRAGMENT_SIZE )
	{
		//change the fragment size
		sReq.m_nFragmentSize = nSize;
		
		//flatten the buffer with the new fragment size
		m->m_cBuffer.Flatten( sReq.m_anBuffer, nSize );
		
		//send to the appserver that a new clip has been added
		if( send_msg( m->m_hServerPort, DR_SET_CLIPBOARD_DATA, &sReq, sizeof( sReq ) - CLIPBOARD_FRAGMENT_SIZE + nSize ) < 0 )
		{
			//error out if the message fails
			dbprintf( "Error: Clipboard::Commit() failed to send DR_SET_CLIPBOARD_DATA to server!\n" );
		}
	}
	else //if the size is > fragment size
	{
		//first declare a buffer with the size
		uint8 *pBuffer = new uint8[nSize];

		//flatten the message with buffer and the size
		m->m_cBuffer.Flatten( pBuffer, nSize );

		//declare an offset
		int nOffset = 0;

		//while their is more to come
		while( nSize > 0 )
		{
			//the current size is the minimum of the fragment size or the current size
			int nCurSize = std::min( CLIPBOARD_FRAGMENT_SIZE, nSize );

			//copy the 
			memcpy( sReq.m_anBuffer, pBuffer + nOffset, nCurSize );
			
			//send the message to the appserver
			if( send_msg( m->m_hServerPort, DR_SET_CLIPBOARD_DATA, &sReq, sizeof( sReq ) - CLIPBOARD_FRAGMENT_SIZE + nCurSize ) != 0 )
			{
				//we have an error, so print out debug info then delete all stuff and return
				dbprintf( "Error: Clipboard::Commit() failed to send buffer to server: %s\n", strerror( errno ) );
				delete[]pBuffer;
				return;
			}
			nSize -= nCurSize;
			nOffset += nCurSize;
		}
		delete[]pBuffer;
	}
}












