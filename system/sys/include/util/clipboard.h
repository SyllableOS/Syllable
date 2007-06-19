/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2003 - 2004 Syllable Team
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

#ifndef __F_GUI_CLIPBOARD_H__
#define __F_GUI_CLIPBOARD_H__

#include <util/string.h>
#include <util/messenger.h>
namespace os
{
#if 0
}
#endif

/** 
 * \ingroup util
 * \par Description:
 *	A clipboard allows a user to save data that is "Cut" or "Copied" from an application.
 *	A clipboard is very useful with TextViews and the such where users have massive amounts of data
 *	that they want to manipulate.
 *
 * \par Usage:
 *	A clipboard can be very confusing to use so this little code can help you out a lot.
 *
 *	\par
 *	If you wish to add a clip to the clipboard you can do:
 *	\code
 *			Clipboard cClipboard;
 *
 *			//we lock the clipboard and then we clear the data's contents
 *			cClipboard.Lock();
 *			cClipboard.Clear();
 *			
 *			//we get the clipboard message and then we add a string to it under "text/plain"
 *			Message *pcData = cClipboard.GetData();
 *			pcData->AddString( "text/plain", *pcBuffer );
 *			
 *			//we committ our changes and then we unlock the clipboard
 *			cClipboard.Commit();
 *			cClipboard.Unlock();
 * \endcode
 * \par
 *	If you wish to get the clipboard contents, you can do:
 * \code
 *		const char *pzBuffer;
 *		int nError;
 *		Clipboard cClipboard;
 *
 *		//lock the clipboard and get the data from the clipboard
 *		cClipboard.Lock();
 *		Message *pcData = cClipboard.GetData();
 *
 *		//the data that is in the clipboard is plain text, so you find a string in the message that is "text/plain"
 *		//nError will be 0 there isn't any strings in the message under the name "text/plain"
 *		//The string "text/plain" will likely change to something a little more generic to allow more than just strings to be clipped. 
 *		nError = pcData->FindString( "text/plain", &pzBuffer );
 *
 *		//add some error checking
 *		if( nError == 0 )
 *		{
 *			//we found the data, so you can do whatever with the data
 *		}
 *
 * \endcode
 * \par
 *	If you wish to monitor the activity of the clipboard you need to monitor its event:
 * \code
 *		m_pcMonitorEvent = new os::Event();
 *		m_pcMonitorEvent->SetToRemote( "os/System/ClipboardHasChanged", -1 );
 *		m_pcMonitorEvent->SetMonitorEnabled( true, this ( your handler), MSG_CLIP_CHANGED (the code you will receive a message under) );
 * \endcode
 *
 * \sa	os::TextView, os::Message, os::Event
 * \author	Kurt Skauen (kurt@atheos.cx)
 * \author	Rick Caudill (rick@syllable.org)
 *****************************************************************************/

class Clipboard
{
public:
    Clipboard(const String& cName="");
    
    ~Clipboard();

    bool	Lock();
    void	Unlock();

    void	Clear();
    void	Commit();
  
    Message* GetData();

private:
	class Private;
	Private* m;
};

}
#endif // __F_GUI_CLIPBOARD_H__



